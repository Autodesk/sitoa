/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "color_utils.h"

#define formula1(in_value) ((2.0f/3.0f) * (1.0f - (in_value -0.5f) * ( in_value - 0.5f)))
#define formula2(in_value) (1.075f - 1.0f / (in_value * 16.0f + 1.0f))

float HLSValue(float in_n1, float in_n2, float in_hue)
{
   // h is 0->1, l 0->1  s 0->1
  
   in_hue = in_hue - std::floor(in_hue);

   if (in_hue < SIXTH) // 60 º
      return (in_n1 + (in_n2 - in_n1) * in_hue / SIXTH );
   else if (in_hue < 3 * SIXTH) // 180 º
      return in_n2;
   else if(in_hue < 4 * SIXTH) // 240 º
      return (in_n1 + (in_n2 - in_n1) * (4 * SIXTH - in_hue) / SIXTH);

   return in_n1;
}

AtRGBA HSVtoRGBA(AtRGBA& hsv)
{
   // h is 0->1, s 0->1  v 0->1
   float f, p, q, t;
   int i;

   AtRGBA rgba = AI_RGBA_ZERO;

   if(hsv.r == -1.0f || hsv.g == 0.0f)
   {
      // achromatic
      rgba.r = hsv.b;
      rgba.g = hsv.b;
      rgba.b = hsv.b;
   }
   else
   {
      // chromatic
      float hue = hsv.r;
      hue = (hue - std::floor(hue));
      hue /= SIXTH;


      i = (int) floor(hue);
      f = hue - i;
      p = hsv.b * (1.0f - (hsv.g));
      q = hsv.b * (1.0f - (hsv.g * f));
      t = hsv.b * (1.0f - (hsv.g * (1.0f - f)));

      switch(i)
      {
         case 0: rgba.r = hsv.b; rgba.g = t; rgba.b = p; break;
         case 1: rgba.r = q; rgba.g = hsv.b; rgba.b = p; break;
         case 2: rgba.r = p; rgba.g = hsv.b; rgba.b = t; break;
         case 3: rgba.r = p; rgba.g = q; rgba.b = hsv.b; break;
         case 4: rgba.r = t; rgba.g = p; rgba.b = hsv.b; break;
         case 5: rgba.r = hsv.b; rgba.g = p; rgba.b = q; break;
      }
   }

   rgba.a = hsv.a; // straight copy for alpha

   return rgba;
}

AtRGBA RGBAtoHSV(AtRGBA& rgba)
{
   // h is 0->1, s 0->1  v 0->1

   AtRGBA hsv = AI_RGBA_ZERO;

   float maxcolor = AiMax(rgba.r, AiMax(rgba.g, rgba.b)); // Luminant delta
   float mincolor = AiMin(rgba.r, AiMin(rgba.g, rgba.b));
   float delta = maxcolor - mincolor;

   hsv.b = maxcolor; // Value

   if( maxcolor == mincolor || maxcolor == 0.0f)
   {
      // Achromatic
      hsv.r = -1.0f;
      hsv.g = 0.0f;
   }
   else
   {
      /* chromatic hue, saturation */

      hsv.g = delta / maxcolor;	

      if (rgba.r == maxcolor)
         hsv.r = (rgba.g - rgba.b) / delta; // between yellow and magenta
      else if (rgba.g == maxcolor)
         hsv.r = 2.0f + ((rgba.b - rgba.r) / delta); // between cyan and yellow
      else
         hsv.r = 4.0f + ((rgba.r - rgba.g) / delta); // between magenta and cyan

      hsv.r *= SIXTH;

      while (hsv.r < 0.0f)
         hsv.r += 1.0f; // make sure it is nonnegative 
   }

   hsv.a = rgba.a; // straight copy for alpha

   return hsv;
}

AtRGBA RGBAtoHLS(AtRGBA& rgba)
{      
   // Given r,g,b 0->1 h is 0->1, l 0->1  s 0->1. If s = 0, h = -1

   AtRGBA hls;
   float delta;

   float cmax = AiMax(rgba.r, AiMax(rgba.g, rgba.b)); // luminant delta
   float cmin = AiMin(rgba.r, AiMin(rgba.g, rgba.b));

   hls.g = (cmax + cmin)*0.5f;

   if (cmax == cmin)
   {
      // achromatic case
      hls.r = -1.0f;
      hls.b = 0.0f;
   } 
   else 
   {
      // chromatic case
      // first calculate the saturation
      if (hls.g < 0.5f)
         hls.b = (cmax - cmin) / (cmax + cmin);
      else
         hls.b = (cmax - cmin) / (2.0f - cmax - cmin);

      delta = cmax - cmin;                    
      
      if (rgba.r == cmax) 
         hls.r = (rgba.g - rgba.b) / delta;           
      else if (rgba.g == cmax) 
         hls.r = 2.0f + (rgba.b - rgba.r) / delta;      
      else 
         hls.r = 4.0f + (rgba.r -rgba.g) / delta;

      hls.r *= SIXTH;
      
      while (hls.r < 0.0f)
         hls.r += 1.0f; // make sure it is nonnegative 
   }

   hls.a = rgba.a; // straight copy for alpha

   return hls;
}

AtRGBA HLStoRGBA(AtRGBA& hls)
{
   // h is 0->1, l 0->1  s 0->1. If s = 0, h = -1

   AtRGBA out_rgba;

   if (hls.b == 0.0f || hls.r == -1.0f)
   {
      out_rgba.r = hls.g;
      out_rgba.g = hls.g;
      out_rgba.b = hls.g;
   }
   else
   {
      float m1,m2;

      if (hls.g <= 0.5f)
         m2 = hls.g * (1.0f + hls.b);
      else
         m2 = hls.g + hls.b - (hls.g * hls.b);

      m1 = 2.0f * hls.g - m2;

      out_rgba.r = HLSValue(m1, m2, hls.r + 1.0f/3.0f); // 120º   
      out_rgba.g = HLSValue(m1, m2, hls.r);         
      out_rgba.b = HLSValue(m1, m2, hls.r - 1.0f/3.0f);   // - 120º
   }
   
   out_rgba.a = hls.a; // straight copy for alpha

   return out_rgba;
}

float BalanceChannel(float in_value, float in_shadow, float in_midtone, float in_highlight)                          
{
   if(in_shadow > 0.0f)
      in_value += in_shadow * formula1(in_value);
   else                                                                     
      in_value += in_shadow * formula2(1.0f - in_value);                                  
   
   in_value = AiClamp(in_value, 0.0f, 1.0f);

   in_value += (in_midtone) * formula1(in_value);
   in_value = AiClamp(in_value, 0.0f, 1.0f);  
                                                                        
   if(in_highlight > 0.0f)
      in_value += in_highlight * formula2(in_value);
   else
      in_value += in_highlight * formula1(in_value);
   
   in_value = AiClamp(in_value, 0.0f, 1.0f);                                                     

   return in_value;
}

// from htoa's color_utils.cpp

AtRGB convertToRGB(const AtRGB& color, int from_space)
{
    if (from_space == COLOR_SPACE_HSV || from_space == COLOR_SPACE_HSL)
    {
        float hue6 = fmod(color.r, 1.0f) * 6.0f;
        float hue2 = hue6;
        if (hue6 > 4.0f) hue2 -= 4.0f;
        else if (hue6 > 2.0f) hue2 -= 2.0f;

        float chroma;
        float sat = AiClamp(color.g, 0.0f, 1.0f);
        if (from_space == COLOR_SPACE_HSV)
            chroma = sat * color.b;
        else // HSL
            chroma = (1.0f - fabsf(2.0f * color.b - 1.0f)) * sat;
      
        float component = chroma * (1.0f - fabsf(hue2 - 1.0f));

        AtRGB rgb;
        if      (hue6 < 1) rgb = AtRGB(chroma, component, 0.0f);
        else if (hue6 < 2) rgb = AtRGB(component, chroma, 0.0f);
        else if (hue6 < 3) rgb = AtRGB(0.0f, chroma, component);
        else if (hue6 < 4) rgb = AtRGB(0.0f, component, chroma);
        else if (hue6 < 5) rgb = AtRGB(component, 0.0f, chroma);
        else               rgb = AtRGB(chroma, 0.0f, component);
      
        float cmin;
        if (from_space == COLOR_SPACE_HSV)
            cmin = color.b - chroma;
        else // HSL
            cmin = color.b - chroma * 0.5f;

        rgb += cmin;
        return rgb;
    }
    else if (from_space == COLOR_SPACE_XYZ || from_space == COLOR_SPACE_XYY)
    {
        // for documentation purposes, CIE->RGB needs color system data, and here
        // are some typical bits that are needed:
        //
        // Name               xRed     yRed     xGreen   yGreen   xBlue    yBlue    White point                Gamma
        // -----------------------------------------------------------------------------------------------------------------------
        // "NTSC",            0.67f,   0.33f,   0.21f,   0.71f,   0.14f,   0.08f,   WhitePoint::IlluminantC,   GAMMA_REC601
        // "EBU (PAL/SECAM)", 0.64f,   0.33f,   0.29f,   0.60f,   0.15f,   0.06f,   WhitePoint::IlluminantD65, GAMMA_REC709
        // "SMPTE",           0.630f,  0.340f,  0.310f,  0.595f,  0.155f,  0.070f,  WhitePoint::IlluminantD65, GAMMA_REC709
        // "HDTV",            0.670f,  0.330f,  0.210f,  0.710f,  0.150f,  0.060f,  WhitePoint::IlluminantD65, GAMMA_REC709 (2.35)
        // "sRGB",            0.670f,  0.330f,  0.210f,  0.710f,  0.150f,  0.060f,  WhitePoint::IlluminantD65, 2.2
        // "CIE",             0.7355f, 0.2645f, 0.2658f, 0.7243f, 0.1669f, 0.0085f, WhitePoint::IlluminantE,   GAMMA_REC709
        // "CIE REC 709",     0.64f,   0.33f,   0.30f,   0.60f,   0.15f,   0.06f,   WhitePoint::IlluminantD65, GAMMA_REC709
        //
        // typical white points are as follows:
        //
        // Name          x            y              Description
        // -----------------------------------------------------------------------------
        // IlluminantA   0.44757f,    0.40745f    Incandescent tungsten
        // IlluminantB   0.34842f,    0.35161f    Obsolete, direct sunlight at noon
        // IlluminantC   0.31006f,    0.31616f    Obsolete, north sky daylight
        // IlluminantD50 0.34567f,    0.35850f    Some print, cameras
        // IlluminantD55 0.33242f,    0.34743f    Some print, cameras
        // IlluminantD65 0.31271f,    0.32902f    For EBU and SMPTE, HDTV, sRGB
        // IlluminantD75 0.29902f,    0.31485f    ???
        // IlluminantE   0.33333333f, 0.33333333f CIE equal-energy illuminant
        // Illuminant93K 0.28480f,    0.29320f    High-efficiency blue phosphor monitors
        // IlluminantF2  0.37207f,    0.37512f    Cool white flourescent (CWF)
        // IlluminantF7  0.31285f,    0.32918f    Broad-band daylight flourescent
        // IlluminantF11 0.38054f,    0.37691f    Narrow-band white flourescent

        // we use the CIE equal-energy color space, as it is the most generic

        float xr, yr, zr, xg, yg, zg, xb, yb, zb;
        float xw, yw, zw;
        float rx, ry, rz, gx, gy, gz, bx, by, bz;
        float rw, gw, bw;

        float xc, yc, zc;
        if (from_space == COLOR_SPACE_XYZ)
        {
            xc = color.r;
            yc = color.g;
            zc = color.b;
        }
        else
        {
            AtRGB xyz = xyYToXYZ(color);
            xc = xyz.r;
            yc = xyz.g;
            zc = xyz.b;
        }

        // these come from the tables above, using CIE equal-energy color space

        xr = 0.7355f; yr = 0.2654f; zr = 1.0f - (xr + yr);
        xg = 0.2658f; yg = 0.7243f; zg = 1.0f - (xg + yg);
        xb = 0.1669f; yb = 0.0085f; zb = 1.0f - (xb + yb);

        xw = 1.0f / 3.0f; yw = 1.0f / 3.0f; zw = 1.0f - (xw + yw);

        // xyz -> rgb matrix, before scaling to white

        rx = (yg * zb) - (yb * zg);  ry = (xb * zg) - (xg * zb);  rz = (xg * yb) - (xb * yg);
        gx = (yb * zr) - (yr * zb);  gy = (xr * zb) - (xb * zr);  gz = (xb * yr) - (xr * yb);
        bx = (yr * zg) - (yg * zr);  by = (xg * zr) - (xr * zg);  bz = (xr * yg) - (xg * yr);

        // white scaling factors; dividing by yw scales the white luminance to unity, as conventional

        rw = ((rx * xw) + (ry * yw) + (rz * zw)) / yw;
        gw = ((gx * xw) + (gy * yw) + (gz * zw)) / yw;
        bw = ((bx * xw) + (by * yw) + (bz * zw)) / yw;

        // xyz -> rgb matrix, correctly scaled to white

        rx = rx / rw;  ry = ry / rw;  rz = rz / rw;
        gx = gx / gw;  gy = gy / gw;  gz = gz / gw;
        bx = bx / bw;  by = by / bw;  bz = bz / bw;

        // rgb of the desired point

        AtRGB result ( (rx * xc) + (ry * yc) + (rz * zc),
                       (gx * xc) + (gy * yc) + (gz * zc),
                       (bx * xc) + (by * yc) + (bz * zc) );

        return result;
    }
   
    // was RGB already (or unknown color space)
    return color;
}

AtRGB convertFromRGB(const AtRGB& color, int to_space)
{
    if (to_space == COLOR_SPACE_HSL || to_space == COLOR_SPACE_HSV)
    {
        float cmax = AiColorMaxRGB(color);
        float cmin = AiMin(AiMin(color.r, color.g), color.b);
        float chroma = cmax - cmin;

        float hue;
        if (chroma == 0.0f)
            hue = 0.0f;
        else if (cmax == color.r)
            hue = (color.g - color.b) / chroma;
        else if (cmax == color.g)
            hue = (color.b - color.r) / chroma + 2.0f;
        else
            hue = (color.r - color.g) / chroma + 4.0f;
        hue *= 1.0f / 6.0f;
        if (hue < 0.0f)
           hue += 1.0f;

        if (to_space == COLOR_SPACE_HSL)
        {
            float lightness = (cmax + cmin) * 0.5f;
            float saturation;
            if (chroma == 0.0f)
                saturation = 0.0f;
            else
                saturation = chroma / (1.0f - fabsf(2.0f * lightness - 1.0f));

            AtRGB result(hue, saturation, lightness);
            return result;
        }
        else
        {
            // HSV
            float value = cmax;
            float saturation = chroma == 0.0f ? 0.0f : chroma / value;
            AtRGB result(hue, saturation, value);
            return result;
        }
    }
    else if (to_space == COLOR_SPACE_XYZ || to_space == COLOR_SPACE_XYY)
    {
        float X = (0.49f * color.r + 0.31f * color.g + 0.2f * color.b) / 0.17697f;
        float Y = (0.17697f * color.r + 0.81240f * color.g + 0.01063f * color.b) / 0.17697f;
        float Z = (0.0f * color.r + 0.01f * color.g + 0.99f * color.b) / 0.17697f;
        AtRGB result;
        if (to_space == COLOR_SPACE_XYZ)
            result = AtRGB(X, Y, Z);
        else // xyY
        {
            float sum = X + Y + Z;
            if (sum > 0.00001f)
                result = AtRGB(X / sum, Y / sum, Y);
            else
                result = AI_RGB_BLACK;
        }
        return result;
    }

    // was RGB already (or unknown color space)
    return color;
}



