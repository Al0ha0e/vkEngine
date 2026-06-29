#ifndef GLYPH_MANAGER_H
#define GLYPH_MANAGER_H

#include <render/buffer.hpp>
#include <array>
#include <limits>
#include <memory>
#include <vector>

namespace vke_render
{
    using GlyphID = uint32_t;
    static constexpr GlyphID INVALID_GLYPH_ID = std::numeric_limits<GlyphID>::max();

    struct GlyphInstanceGPU
    {
        glm::vec4 quadRect;
        glm::vec4 uvRect;
        glm::vec4 color;
        glm::uvec4 atlasInfo;
    };

    constexpr uint32_t GLYPHS_PER_BUFFER = 512;
    constexpr uint32_t MAX_GLYPH_BUFFER_CNT = 16;
    constexpr uint32_t MAX_GLYPH_CNT = GLYPHS_PER_BUFFER * MAX_GLYPH_BUFFER_CNT;
    constexpr VkDeviceSize GLYPH_BUFFER_SIZE = sizeof(GlyphInstanceGPU) * GLYPHS_PER_BUFFER;
    constexpr VkDeviceSize GLYPH_DATA_SIZE = sizeof(GlyphInstanceGPU) * MAX_GLYPH_CNT;

    inline uint32_t GetGlyphBufferIndex(GlyphID glyphID) { return glyphID >> 9; }
    inline uint32_t GetGlyphSlotIndex(GlyphID glyphID) { return glyphID & (GLYPHS_PER_BUFFER - 1); }
    inline VkDeviceSize GetGlyphBufferOffset(uint32_t bufferIndex) { return GLYPH_BUFFER_SIZE * bufferIndex; }

    struct CPUGlyphData
    {
        HostCoherentBuffer cpuGlyphs;
        std::array<uint32_t, MAX_GLYPH_BUFFER_CNT> updateCnts{};
        std::vector<GlyphID> freeGlyphs;

        CPUGlyphData()
            : cpuGlyphs(GLYPH_DATA_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
        {
            freeGlyphs.reserve(MAX_GLYPH_CNT);
            for (uint32_t bufferIndex = MAX_GLYPH_BUFFER_CNT; bufferIndex-- > 0;)
                for (uint32_t slot = GLYPHS_PER_BUFFER; slot-- > 0;)
                    freeGlyphs.push_back((bufferIndex << 9) | slot);
        }

        void Allocate(const std::vector<GlyphInstanceGPU> &glyphs, std::vector<GlyphID> &glyphIDs)
        {
            glyphIDs.clear();
            glyphIDs.reserve(glyphs.size());
            for (const GlyphInstanceGPU &glyph : glyphs)
            {
                GlyphID glyphID = freeGlyphs.back();
                freeGlyphs.pop_back();
                const uint32_t bufferIndex = GetGlyphBufferIndex(glyphID);
                glyphAt(glyphID) = glyph;
                markDirty(bufferIndex);
                glyphIDs.push_back(glyphID);
            }
        }
        bool CanAllocate(size_t glyphCount, size_t reclaimCount) const
        {
            return glyphCount <= freeGlyphs.size() + reclaimCount;
        }
        void Update(GlyphID glyphID, const GlyphInstanceGPU &glyph)
        {
            const uint32_t bufferIndex = GetGlyphBufferIndex(glyphID);
            glyphAt(glyphID) = glyph;
            markDirty(bufferIndex);
        }
        void UpdateColor(GlyphID glyphID, const glm::vec4 &color)
        {
            const uint32_t bufferIndex = GetGlyphBufferIndex(glyphID);
            glyphAt(glyphID).color = color;
            markDirty(bufferIndex);
        }
        void Release(const std::vector<GlyphID> &glyphIDs)
        {
            for (GlyphID glyphID : glyphIDs)
                freeGlyphs.push_back(glyphID);
        }
        void Clear()
        {
            updateCnts.fill(MAX_FRAMES_IN_FLIGHT);
            freeGlyphs.clear();
            freeGlyphs.reserve(MAX_GLYPH_CNT);
            for (uint32_t bufferIndex = MAX_GLYPH_BUFFER_CNT; bufferIndex-- > 0;)
                for (uint32_t slot = GLYPHS_PER_BUFFER; slot-- > 0;)
                    freeGlyphs.push_back((bufferIndex << 9) | slot);
        }

    private:
        GlyphInstanceGPU &glyphAt(GlyphID glyphID)
        {
            return reinterpret_cast<GlyphInstanceGPU *>(cpuGlyphs.data)[glyphID];
        }

        void markDirty(uint32_t bufferIndex)
        {
            updateCnts[bufferIndex] = MAX_FRAMES_IN_FLIGHT;
        }
    };

    class GlyphManager
    {
    public:
        GlyphManager() : cpuGlyphData(std::make_shared<CPUGlyphData>())
        {
            for (uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
                deviceBuffers[frame] = std::make_unique<DeviceBuffer>(GLYPH_DATA_SIZE, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        }

        void Sync(uint32_t currentFrame)
        {
            for (uint32_t bufferIndex = 0; bufferIndex < MAX_GLYPH_BUFFER_CNT; ++bufferIndex)
            {
                if (cpuGlyphData->updateCnts[bufferIndex] == 0)
                    continue;
                --cpuGlyphData->updateCnts[bufferIndex];
                const VkDeviceSize offset = GetGlyphBufferOffset(bufferIndex);
                RenderEnvironment::CopyBuffer(cpuGlyphData->cpuGlyphs.buffer, deviceBuffers[currentFrame]->buffer,
                                              GLYPH_BUFFER_SIZE, offset, offset);
            }
        }
        const std::shared_ptr<CPUGlyphData> &GetCPUGlyphData() const { return cpuGlyphData; }

        void LoadSceneGlyphData(std::shared_ptr<CPUGlyphData> glyphData)
        {
            cpuGlyphData = glyphData == nullptr ? std::make_shared<CPUGlyphData>() : std::move(glyphData);
            cpuGlyphData->updateCnts.fill(MAX_FRAMES_IN_FLIGHT);
        }

        std::shared_ptr<CPUGlyphData> ToSceneGlyphData() const
        {
            return cpuGlyphData;
        }

        void ClearGlyphs()
        {
            cpuGlyphData = std::make_shared<CPUGlyphData>();
            cpuGlyphData->updateCnts.fill(MAX_FRAMES_IN_FLIGHT);
        }

        VkDescriptorBufferInfo GetDescriptorBufferInfo(uint32_t currentFrame) const
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = deviceBuffers[currentFrame]->buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = GLYPH_DATA_SIZE;
            return bufferInfo;
        }

    private:
        std::shared_ptr<CPUGlyphData> cpuGlyphData;
        std::unique_ptr<DeviceBuffer> deviceBuffers[MAX_FRAMES_IN_FLIGHT];
    };
}

#endif
