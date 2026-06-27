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

    struct CPUGlyphData
    {
        static constexpr uint32_t GLYPHS_PER_BUFFER = 512;
        static constexpr uint32_t MAX_GLYPH_BUFFER_CNT = 16;
        static constexpr uint32_t MAX_GLYPH_CNT = GLYPHS_PER_BUFFER * MAX_GLYPH_BUFFER_CNT;

        std::array<std::array<GlyphInstanceGPU, GLYPHS_PER_BUFFER>, MAX_GLYPH_BUFFER_CNT> cpuGlyphs{};
        std::array<uint32_t, MAX_GLYPH_BUFFER_CNT> updateCnts{};
        std::vector<GlyphID> freeGlyphs;

        CPUGlyphData()
        {
            freeGlyphs.reserve(MAX_GLYPH_CNT);
            for (uint32_t bufferIndex = MAX_GLYPH_BUFFER_CNT; bufferIndex-- > 0;)
                for (uint32_t slot = GLYPHS_PER_BUFFER; slot-- > 0;)
                    freeGlyphs.push_back((bufferIndex << 9) | slot);
        }

        bool Allocate(const std::vector<GlyphInstanceGPU> &glyphs, std::vector<GlyphID> &glyphIDs)
        {
            if (glyphs.size() > freeGlyphs.size())
                return false;

            glyphIDs.clear();
            glyphIDs.reserve(glyphs.size());
            for (const GlyphInstanceGPU &glyph : glyphs)
            {
                GlyphID glyphID = freeGlyphs.back();
                freeGlyphs.pop_back();
                const uint32_t bufferIndex = GetBufferIndex(glyphID);
                cpuGlyphs[bufferIndex][GetSlotIndex(glyphID)] = glyph;
                markDirty(bufferIndex);
                glyphIDs.push_back(glyphID);
            }
            return true;
        }
        bool CanAllocate(size_t glyphCount, size_t reclaimCount) const
        {
            return glyphCount <= freeGlyphs.size() + reclaimCount;
        }
        void Update(GlyphID glyphID, const GlyphInstanceGPU &glyph)
        {
            const uint32_t bufferIndex = GetBufferIndex(glyphID);
            const uint32_t slotIndex = GetSlotIndex(glyphID);
            cpuGlyphs[bufferIndex][slotIndex] = glyph;
            markDirty(bufferIndex);
        }
        void UpdateColor(GlyphID glyphID, const glm::vec4 &color)
        {
            const uint32_t bufferIndex = GetBufferIndex(glyphID);
            cpuGlyphs[bufferIndex][GetSlotIndex(glyphID)].color = color;
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

        static uint32_t GetBufferIndex(GlyphID glyphID) { return glyphID >> 9; }
        static uint32_t GetSlotIndex(GlyphID glyphID) { return glyphID & (GLYPHS_PER_BUFFER - 1); }

    private:
        void markDirty(uint32_t bufferIndex)
        {
            updateCnts[bufferIndex] = MAX_FRAMES_IN_FLIGHT;
        }
    };

    class GlyphManager
    {
    public:
        static constexpr uint32_t GLYPHS_PER_BUFFER = CPUGlyphData::GLYPHS_PER_BUFFER;
        static constexpr uint32_t MAX_GLYPH_BUFFER_CNT = CPUGlyphData::MAX_GLYPH_BUFFER_CNT;
        static constexpr uint32_t MAX_GLYPH_CNT = CPUGlyphData::MAX_GLYPH_CNT;

        GlyphManager() : cpuGlyphData(std::make_shared<CPUGlyphData>())
        {
            constexpr VkDeviceSize bufferSize = sizeof(GlyphInstanceGPU) * GLYPHS_PER_BUFFER;
            for (uint32_t bufferIndex = 0; bufferIndex < MAX_GLYPH_BUFFER_CNT; ++bufferIndex)
                for (uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
                    deviceBuffers[bufferIndex][frame] = std::make_unique<DeviceBuffer>(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        }

        bool Allocate(const std::vector<GlyphInstanceGPU> &glyphs, std::vector<GlyphID> &glyphIDs)
        {
            return cpuGlyphData->Allocate(glyphs, glyphIDs);
        }
        bool CanAllocate(size_t glyphCount, size_t reclaimCount = 0) const
        {
            return cpuGlyphData->CanAllocate(glyphCount, reclaimCount);
        }
        void Update(GlyphID glyphID, const GlyphInstanceGPU &glyph)
        {
            cpuGlyphData->Update(glyphID, glyph);
        }
        void UpdateColor(GlyphID glyphID, const glm::vec4 &color)
        {
            cpuGlyphData->UpdateColor(glyphID, color);
        }
        void Release(const std::vector<GlyphID> &glyphIDs)
        {
            cpuGlyphData->Release(glyphIDs);
        }
        void Sync(uint32_t currentFrame)
        {
            for (uint32_t bufferIndex = 0; bufferIndex < MAX_GLYPH_BUFFER_CNT; ++bufferIndex)
            {
                if (cpuGlyphData->updateCnts[bufferIndex] == 0)
                    continue;
                --cpuGlyphData->updateCnts[bufferIndex];
                deviceBuffers[bufferIndex][currentFrame]->ToBuffer(
                    0, cpuGlyphData->cpuGlyphs[bufferIndex].data(), sizeof(GlyphInstanceGPU) * GLYPHS_PER_BUFFER);
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

        VkDescriptorBufferInfo GetDescriptorBufferInfo(uint32_t bufferIndex, uint32_t currentFrame) const
        {
            return deviceBuffers[bufferIndex][currentFrame]->GetDescriptorBufferInfo();
        }

        static uint32_t GetBufferIndex(GlyphID glyphID) { return glyphID >> 9; }
        static uint32_t GetSlotIndex(GlyphID glyphID) { return glyphID & (GLYPHS_PER_BUFFER - 1); }

    private:
        std::shared_ptr<CPUGlyphData> cpuGlyphData;
        std::unique_ptr<DeviceBuffer> deviceBuffers[MAX_GLYPH_BUFFER_CNT][MAX_FRAMES_IN_FLIGHT];
    };
}

#endif
