/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <ai.h>
#include <algorithm>

#define SIXTH 0.1666666667f

AtRGBA RGBAtoHSV(AtRGBA& rgba);
AtRGBA HSVtoRGBA(AtRGBA& hsv);

AtRGBA RGBAtoHLS(AtRGBA& rgba);
AtRGBA HLStoRGBA(AtRGBA& hsv);

float BalanceChannel(float in_value, float in_shadow, float in_midtone, float in_highlight);

// from htoa's color_utils.h

#define COLOR_SPACE_RGB 0
#define COLOR_SPACE_XYZ 1
#define COLOR_SPACE_XYY 2
#define COLOR_SPACE_HSL 3
#define COLOR_SPACE_HSV 4

inline AtRGB XYZToxyY(const AtRGB& xyz)
{
    AtRGB result;
    float sum = xyz.r + xyz.g + xyz.b;
    if (sum > 0.00001f)
    {
        result.r = xyz.r / sum;
        result.g = xyz.g / sum;
        result.b = xyz.g;
    }
    else
        result = AI_RGB_BLACK;
    return result;
}

inline AtRGB xyYToXYZ(const AtRGB& xyY)
{
    AtRGB result(xyY.b * xyY.r / xyY.g,
                  xyY.b,
                  xyY.b * (1.0f - xyY.r - xyY.g) / xyY.g);
    return result;
}

inline void RGBAGamma(AtRGBA* color, float gamma)
{
   if (gamma == 1.0f || gamma == 0.0f) 
      return;
   float inv_gamma = 1.0f / gamma;
   color->r = AiFastPow(color->r, inv_gamma);
   color->g = AiFastPow(color->g, inv_gamma);
   color->b = AiFastPow(color->b, inv_gamma);
   color->a = AiFastPow(color->a, inv_gamma);
}

AtRGB convertToRGB(const AtRGB& color, int from_space);
AtRGB convertFromRGB(const AtRGB& color, int to_space);

