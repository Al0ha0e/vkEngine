#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

namespace vke_ui
{
    struct TextLine
    {
        size_t begin, end;
        float width;
    };

    struct TextLayout
    {
        std::vector<TextLine> lines;
        float width = 0;
        float height = 0;
        std::optional<float> wrapWidth;
    };

    template <typename Characters>
    TextLayout LayoutText(const Characters &characters, float scale, float lineHeight,
                          std::optional<float> maxWidth = {})
    {
        TextLayout result;
        result.wrapWidth = maxWidth;
        if (characters.empty())
            return result;

        std::vector<double> advances(characters.size() + 1, 0);
        for (size_t i = 0; i < characters.size(); ++i)
            advances[i + 1] = advances[i] + characters[i].advance * static_cast<double>(scale);
        auto width = [&](size_t begin, size_t end)
        { return static_cast<float>(advances[end] - advances[begin]); };
        auto space = [&](size_t i)
        { return characters[i].codepoint == ' ' || characters[i].codepoint == '\t'; };
        auto append = [&](size_t begin, size_t end)
        {
            while (end > begin && space(end - 1))
                --end;
            const float lineWidth = width(begin, end);
            result.lines.push_back({begin, end, lineWidth});
            result.width = std::max(result.width, lineWidth);
        };

        size_t paragraph = 0;
        while (paragraph <= characters.size())
        {
            size_t end = paragraph;
            while (end < characters.size() && characters[end].codepoint != '\n')
                ++end;
            size_t begin = paragraph;
            size_t lineEnd = begin;
            size_t cursor = begin;
            while (cursor < end)
            {
                while (cursor < end && space(cursor))
                    ++cursor;
                const size_t wordBegin = cursor;
                while (cursor < end && !space(cursor))
                    ++cursor;
                if (wordBegin == cursor)
                    break;

                if (maxWidth && lineEnd > begin && width(begin, cursor) > *maxWidth)
                {
                    append(begin, lineEnd);
                    begin = wordBegin;
                }
                if (maxWidth)
                    for (size_t i = begin > lineEnd ? begin : lineEnd; i < cursor; ++i)
                        if (i > begin && width(begin, i + 1) > *maxWidth)
                        {
                            append(begin, i);
                            begin = i;
                        }
                lineEnd = cursor;
            }
            append(begin, lineEnd);
            if (end == characters.size())
                break;
            paragraph = end + 1;
        }
        result.height = static_cast<float>(result.lines.size()) * lineHeight * scale;
        return result;
    }
}
