import sys
import json
from PIL import Image
from pyglm import glm

class Param:
    def __init__(self):
        self.SeaLevel = 0.0
        self.PlanetRadius = 6360000.0
        self.AtmosphereHeight = 60000.0
        self.SunLightIntensity = 31.4
        self.SunLightColor = glm.vec3(1.0, 1.0, 1.0)
        self.SunDiskAngle = 9.0
        self.RayleighScatteringScale = 1.0
        self.RayleighScatteringScalarHeight = 8000.0
        self.MieScatteringScale = 1.0
        self.MieAnisotropy = 0.8
        self.MieScatteringScalarHeight = 1200.0
        self.OzoneAbsorptionScale = 1.0
        self.OzoneLevelCenterHeight = 25000.0
        self.OzoneLevelWidth = 15000.0
        self.AerialPerspectiveDistance = 32000.0
    def loadFromJSON(self, settings):
        self.SeaLevel = settings["SeaLevel"]
        self.PlanetRadius = settings["PlanetRadius"]
        self.AtmosphereHeight = settings["AtmosphereHeight"]    
        self.SunLightIntensity = settings["SunLightIntensity"]
        self.SunLightColor = glm.vec3(settings["SunLightColor"][0], settings["SunLightColor"][1], settings["SunLightColor"][2])
        self.SunDiskAngle = settings["SunDiskAngle"]
        self.RayleighScatteringScale = settings["RayleighScatteringScale"]
        self.RayleighScatteringScalarHeight = settings["RayleighScatteringScalarHeight"]
        self.MieScatteringScale = settings["MieScatteringScale"]
        self.MieAnisotropy = settings["MieAnisotropy"]
        self.MieScatteringScalarHeight = settings["MieScatteringScalarHeight"]
        self.OzoneAbsorptionScale = settings["OzoneAbsorptionScale"]
        self.OzoneLevelCenterHeight = settings["OzoneLevelCenterHeight"]
        self.OzoneLevelWidth = settings["OzoneLevelWidth"]
        self.AerialPerspectiveDistance = settings["AerialPerspectiveDistance"]


def RayleighCoefficient(param, h):
    sigma = glm.vec3(5.802, 13.558, 33.1) * 1e-6
    H_R = param.RayleighScatteringScalarHeight
    rho_h = glm.exp(-(h / H_R))
    return sigma * rho_h

def RayleiPhase(param, cos_theta):
    return (3.0 / (16.0 * glm.pi())) * (1.0 + cos_theta * cos_theta)

def MieCoefficient(param, h):
    sigma = glm.vec3(3.996 * 1e-6).xxx
    H_M = param.MieScatteringScalarHeight
    rho_h = glm.exp(-(h / H_M))
    return sigma * rho_h

def MiePhase(param, cos_theta):
    g = param.MieAnisotropy
    a = 3.0 / (8.0 * glm.pi())
    b = (1.0 - g*g) / (2.0 + g*g)
    c = 1.0 + cos_theta*cos_theta
    d = glm.pow(1.0 + g*g - 2*g*cos_theta, 1.5)
    
    return a * b * (c / d)

def MieAbsorption(param, h):
    sigma = glm.vec3(4.4 * 1e-6).xxx
    H_M = param.MieScatteringScalarHeight
    rho_h = glm.exp(-(h / H_M))
    return sigma * rho_h

def OzoneAbsorption(param, h):
    sigma_lambda = glm.vec3(0.650, 1.881, 0.085) * 1e-6
    center = param.OzoneLevelCenterHeight
    width = param.OzoneLevelWidth
    rho = max(0, 1.0 - (abs(h - center) / width))
    return sigma_lambda * rho

def Scattering(param, p, lightDir, viewDir):
    cos_theta = glm.dot(lightDir, viewDir)
    h = glm.length(p) - param.PlanetRadius
    rayleigh = RayleighCoefficient(param, h) * RayleiPhase(param, cos_theta)
    mie = MieCoefficient(param, h) * MiePhase(param, cos_theta)
    return rayleigh + mie;

def Transmittance(param, p1, p2):
    N_SAMPLE = 32

    dir = glm.normalize(p2 - p1)
    distance = glm.length(p2 - p1)
    ds = distance / float(N_SAMPLE)
    sum = 0.0
    p = p1 + (dir * ds) * 0.5

    for i in range(0, N_SAMPLE):
        h = glm.length(p) - param.PlanetRadius

        scattering = RayleighCoefficient(param, h) + MieCoefficient(param, h)
        absorption = OzoneAbsorption(param, h) + MieAbsorption(param, h)
        extinction = scattering + absorption

        sum += extinction * ds
        p += dir * ds

    return glm.exp(-sum)

def UvToTransmittanceLutParams(bottomRadius, topRadius, uv):
    x_mu = uv.x
    x_r = uv.y

    H = glm.sqrt(max(0.0, topRadius * topRadius - bottomRadius * bottomRadius))
    rho = H * x_r
    r = glm.sqrt(max(0.0, rho * rho + bottomRadius * bottomRadius))

    d_min = topRadius - r
    d_max = rho + H
    d = d_min + x_mu * (d_max - d_min)
    mu = 1.0 if d == 0.0 else (H * H - rho * rho - d * d) / (2.0 * r * d)
    mu = glm.clamp(mu, -1.0, 1.0)
    return (mu, r)

def RayIntersectSphere(center, radius, rayStart, rayDir):
    OS = glm.length(center - rayStart)
    SH = glm.dot(center - rayStart, rayDir)
    OH = glm.sqrt(OS*OS - SH*SH)
    PH = glm.sqrt(radius*radius - OH*OH)

    # ray miss sphere
    if OH > radius:
        return -1

    # use min distance
    t1 = SH - PH
    t2 = SH + PH
    t =  t2 if t1 < 0 else t1

    return t

def GetTransmittanceLutUv(bottomRadius, topRadius, mu, r):
    H = glm.sqrt(max(0.0, topRadius * topRadius - bottomRadius * bottomRadius))
    rho = glm.sqrt(max(0.0, r * r - bottomRadius * bottomRadius))
    discriminant = r * r * (mu * mu - 1.0) + topRadius * topRadius
    d = max(0.0, (-r * mu + glm.sqrt(discriminant)))
    d_min = topRadius - r
    d_max = rho + H
    x_mu = (d - d_min) / (d_max - d_min)
    x_r = rho / H
    return glm.vec2(x_mu, x_r)

def LinearClampSample(texture, uv):
    x = uv.x * (texture.width - 1)
    y = uv.y * (texture.height - 1)
    uv = uv * glm.vec2(texture.width,texture.height)
    x0 = int(x)
    y0 = int(y)
    x1 = min(x0 + 1, texture.width - 1)
    y1 = min(y0 + 1, texture.height - 1)

    c00 = glm.vec3(texture.getpixel((x0, y0))) / 255.0
    c01 = glm.vec3(texture.getpixel((x0, y1))) / 255.0
    c10 = glm.vec3(texture.getpixel((x1, y0))) / 255.0
    c11 = glm.vec3(texture.getpixel((x1, y1))) / 255.0

    fx = x - x0
    fy = y - y0

    return glm.lerp(glm.lerp(c00, c10, fx), glm.lerp(c01, c11, fx), fy) 

# 查表计算任意点 p 沿着任意方向 dir 到大气层边缘的 transmittance
def TransmittanceToAtmosphere(param, p, dir, lut : Image.Image):
    bottomRadius = param.PlanetRadius
    topRadius = param.PlanetRadius + param.AtmosphereHeight
    upVector = glm.normalize(p)
    cos_theta = glm.dot(upVector, dir)
    r = glm.length(p)
    uv = GetTransmittanceLutUv(bottomRadius, topRadius, cos_theta, r)
    uv.x = glm.clamp(uv.x,0,1)#31)
    uv.y = glm.clamp(uv.y,0,1)

    # color = lut.getpixel([int(uv.x * (lut.width-1)), int(uv.y * (lut.height-1))]) #int(uv.x), (lut.height-1) - int(uv.y * (lut.height-1))
    # ret = glm.vec3(float(color[0]) / 255, float(color[1]) / 255, float(color[2]) / 255) 
    return LinearClampSample(lut, uv)


# 积分计算多重散射查找表
def IntegralMultiScattering(param, samplePoint, lightDir,  _transmittanceLut):
    N_DIRECTION = 64
    N_SAMPLE = 32
    RandomSphereSamples = [
        glm.vec3(-0.7838,-0.620933,0.00996137),
        glm.vec3(0.106751,0.965982,0.235549),
        glm.vec3(-0.215177,-0.687115,-0.693954),
        glm.vec3(0.318002,0.0640084,-0.945927),
        glm.vec3(0.357396,0.555673,0.750664),
        glm.vec3(0.866397,-0.19756,0.458613),
        glm.vec3(0.130216,0.232736,-0.963783),
        glm.vec3(-0.00174431,0.376657,0.926351),
        glm.vec3(0.663478,0.704806,-0.251089),
        glm.vec3(0.0327851,0.110534,-0.993331),
        glm.vec3(0.0561973,0.0234288,0.998145),
        glm.vec3(0.0905264,-0.169771,0.981317),
        glm.vec3(0.26694,0.95222,-0.148393),
        glm.vec3(-0.812874,-0.559051,-0.163393),
        glm.vec3(-0.323378,-0.25855,-0.910263),
        glm.vec3(-0.1333,0.591356,-0.795317),
        glm.vec3(0.480876,0.408711,0.775702),
        glm.vec3(-0.332263,-0.533895,-0.777533),
        glm.vec3(-0.0392473,-0.704457,-0.708661),
        glm.vec3(0.427015,0.239811,0.871865),
        glm.vec3(-0.416624,-0.563856,0.713085),
        glm.vec3(0.12793,0.334479,-0.933679),
        glm.vec3(-0.0343373,-0.160593,-0.986423),
        glm.vec3(0.580614,0.0692947,0.811225),
        glm.vec3(-0.459187,0.43944,0.772036),
        glm.vec3(0.215474,-0.539436,-0.81399),
        glm.vec3(-0.378969,-0.31988,-0.868366),
        glm.vec3(-0.279978,-0.0109692,0.959944),
        glm.vec3(0.692547,0.690058,0.210234),
        glm.vec3(0.53227,-0.123044,-0.837585),
        glm.vec3(-0.772313,-0.283334,-0.568555),
        glm.vec3(-0.0311218,0.995988,-0.0838977),
        glm.vec3(-0.366931,-0.276531,-0.888196),
        glm.vec3(0.488778,0.367878,-0.791051),
        glm.vec3(-0.885561,-0.453445,0.100842),
        glm.vec3(0.71656,0.443635,0.538265),
        glm.vec3(0.645383,-0.152576,-0.748466),
        glm.vec3(-0.171259,0.91907,0.354939),
        glm.vec3(-0.0031122,0.9457,0.325026),
        glm.vec3(0.731503,0.623089,-0.276881),
        glm.vec3(-0.91466,0.186904,0.358419),
        glm.vec3(0.15595,0.828193,-0.538309),
        glm.vec3(0.175396,0.584732,0.792038),
        glm.vec3(-0.0838381,-0.943461,0.320707),
        glm.vec3(0.305876,0.727604,0.614029),
        glm.vec3(0.754642,-0.197903,-0.62558),
        glm.vec3(0.217255,-0.0177771,-0.975953),
        glm.vec3(0.140412,-0.844826,0.516287),
        glm.vec3(-0.549042,0.574859,-0.606705),
        glm.vec3(0.570057,0.17459,0.802841),
        glm.vec3(-0.0330304,0.775077,0.631003),
        glm.vec3(-0.938091,0.138937,0.317304),
        glm.vec3(0.483197,-0.726405,-0.48873),
        glm.vec3(0.485263,0.52926,0.695991),
        glm.vec3(0.224189,0.742282,-0.631472),
        glm.vec3(-0.322429,0.662214,-0.676396),
        glm.vec3(0.625577,-0.12711,0.769738),
        glm.vec3(-0.714032,-0.584461,-0.385439),
        glm.vec3(-0.0652053,-0.892579,-0.446151),
        glm.vec3(0.408421,-0.912487,0.0236566),
        glm.vec3(0.0900381,0.319983,0.943135),
        glm.vec3(-0.708553,0.483646,0.513847),
        glm.vec3(0.803855,-0.0902273,0.587942),
        glm.vec3(-0.0555802,-0.374602,-0.925519),
    ]
    uniform_phase = 1.0 / (4.0 * glm.pi())
    sphereSolidAngle = 4.0 * glm.pi() / float(N_DIRECTION)
    
    G_2 = glm.vec3(0, 0, 0)
    f_ms = glm.vec3(0, 0, 0)

    for i in range(0, N_DIRECTION):
        # 光线和大气层求交
        viewDir = RandomSphereSamples[i]
        dis = RayIntersectSphere(glm.vec3(0,0,0), param.PlanetRadius + param.AtmosphereHeight, samplePoint, viewDir)
        d = RayIntersectSphere(glm.vec3(0,0,0), param.PlanetRadius, samplePoint, viewDir)
        if d > 0:
            dis = min(dis, d)
        ds = dis / float(N_SAMPLE)

        p = samplePoint + (viewDir * ds) * 0.5
        opticalDepth = glm.vec3(0, 0, 0)

        for j in range(0,N_SAMPLE):
            h = glm.length(p) - param.PlanetRadius
            if h < 0:
                break
                # p += viewDir * ds
                # continue
            sigma_s = RayleighCoefficient(param, h) + MieCoefficient(param, h)  # scattering
            sigma_a = OzoneAbsorption(param, h) + MieAbsorption(param, h)       # absorption
            sigma_t = sigma_s + sigma_a                                         # extinction
            opticalDepth += sigma_t * ds

            t1 = TransmittanceToAtmosphere(param, p, lightDir, _transmittanceLut)
            s  = Scattering(param, p, lightDir, viewDir)
            t2 = glm.exp(-opticalDepth)
            
            # 用 1.0 代替太阳光颜色, 该变量在后续的计算中乘上去
            G_2  += t1 * s * t2 * uniform_phase * ds * 1.0
            f_ms += t2 * sigma_s * uniform_phase * ds
            p += viewDir * ds

    G_2 *= sphereSolidAngle
    f_ms *= sphereSolidAngle

    # print(G_2, f_ms, G_2 * (1.0 / (1.0 - f_ms)))
    return G_2 * (1.0 / (1.0 - f_ms))

def GenTransmittanceLUT(param, path):
    img = Image.new("RGB", (256, 64))
    pixels = img.load()
    for i in range(0,256):
        for j in range(0,64):
            bottomRadius = param.PlanetRadius
            topRadius = param.PlanetRadius + param.AtmosphereHeight

            uv = glm.vec2(i / 256, j / 64)
            # 计算当前 uv 对应的 cos_theta, height
            cos_theta, r = UvToTransmittanceLutParams(bottomRadius, topRadius, uv)

            sin_theta = glm.sqrt(1.0 - cos_theta * cos_theta)
            viewDir = glm.vec3(sin_theta, cos_theta, 0)
            eyePos = glm.vec3(0, r, 0)

            # 光线和大气层求交
            dis = RayIntersectSphere(glm.vec3(0,0,0), param.PlanetRadius + param.AtmosphereHeight, eyePos, viewDir)
            hitPoint = eyePos + viewDir * dis

            color = Transmittance(param, eyePos, hitPoint)
            pixels[i, 63 - j] = (int(color.x * 255), int(color.y * 255), int(color.z * 255))
    img.save(path + "transmitance_lut.png", "PNG")
    return img

def filterNan(val):
    if glm.isnan(val):
        return 0
    return val

def GenMultiScatteringLUT(param, path, transmittance_LUT):
    img = Image.new("RGB", (32, 32))
    pixels = img.load()
    for i in range(0,32):
        for j in range(0,32):
            print(i,j)
            uv = glm.vec2(i / 32.0, j / 32.0)
            mu_s = uv.x * 2.0 - 1.0
            r = uv.y * param.AtmosphereHeight + param.PlanetRadius
            cos_theta = mu_s
            sin_theta = glm.sqrt(1.0 - cos_theta * cos_theta)
            lightDir = glm.vec3(sin_theta, cos_theta, 0)
            p = glm.vec3(0, r, 0)
            color = IntegralMultiScattering(param, p, lightDir, transmittance_LUT)
            #pixels[i, 31 - j] = (glm.clamp(int(filterNan(color.x) * 255 * 50), 0,255), glm.clamp(int(filterNan(color.y) * 255 * 50), 0,255), glm.clamp(int(filterNan(color.z) * 255 * 50), 0,255))
            pixels[i, 31 - j] = (glm.clamp(int(filterNan(color.x) * 255), 0,255), glm.clamp(int(filterNan(color.y) * 255), 0,255), glm.clamp(int(filterNan(color.z) * 255), 0,255))
    img.save(path + "scattering_lut.png", "PNG")
    return img


param = Param()

with open(sys.argv[2], 'r') as f:
    settings = f.read()

settings = json.loads(settings)

param.loadFromJSON(settings)

transmittance_LUT = GenTransmittanceLUT(param, sys.argv[1])
multiscattering_LUT = GenMultiScatteringLUT(param, sys.argv[1], transmittance_LUT)