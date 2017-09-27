/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>
#include <stdio.h>

////////////////////////////
// XSI Shaders
////////////////////////////

// Texture
extern const AtNodeMethods* mib_texture_remapMethods;
extern const AtNodeMethods* txt2d_image_explicitMethods;
extern const AtNodeMethods* txt2d_scalarimage_explicitMethods;
extern const AtNodeMethods* txt2d_gradient_v2Methods;
extern const AtNodeMethods* txt3d_cellular_v3Methods;
extern const AtNodeMethods* txt3d_checkerboardMethods;
extern const AtNodeMethods* txt3d_marbleMethods;
extern const AtNodeMethods* sib_texture_marbleMethods;
extern const AtNodeMethods* SIBTextureSnowMethods;
extern const AtNodeMethods* TXT3DTextureSnowMethods;
extern const AtNodeMethods* SIBTextureGridMethods;
extern const AtNodeMethods* TXT2DTextureGridMethods;

// Attributes
extern const AtNodeMethods* SIBAttributeBooleanMethods;
extern const AtNodeMethods* SIBAttributeColorMethods;
extern const AtNodeMethods* SIBAttributeIntegerMethods;
extern const AtNodeMethods* SIBAttributeScalarMethods;
extern const AtNodeMethods* SIBAttributeVectorMethods;
extern const AtNodeMethods* SIBAttributeTransformMethods;

// Bump
extern const AtNodeMethods* SIBZBumpMethods;

// Color Channels
extern const AtNodeMethods* SIBChannelPickerMethods;
extern const AtNodeMethods* SIBHLSACombineMethods;
extern const AtNodeMethods* SIBHSVACombineMethods;
extern const AtNodeMethods* SIBColorCombineMethods;

// Image Processing
extern const AtNodeMethods* SIBColorCorrectionMethods;
extern const AtNodeMethods* SIBColorBalanceMethods;
extern const AtNodeMethods* SIBColorHLSAdjustMethods;
extern const AtNodeMethods* MIBColorIntensityMethods;
extern const AtNodeMethods* SIBColorInvertMethods;
extern const AtNodeMethods* rgba_keyerMethods;
extern const AtNodeMethods* SIBScalarInvertMethods;

// Mixers
extern const AtNodeMethods* SIBColorMixMethods;
extern const AtNodeMethods* SIBColorGradientMethods;

// Conversion
extern const AtNodeMethods* SIBColorToScalarMethods;
extern const AtNodeMethods* SIBColorToVectorMethods;
extern const AtNodeMethods* SIBColorToBooleanMethods;
extern const AtNodeMethods* SIBColorRGBToHSVMethods;
extern const AtNodeMethods* SIBColorHSVToRGBMethods;
extern const AtNodeMethods* SIBIntegerToScalarMethods;
extern const AtNodeMethods* SIBScalarToColorMethods;
extern const AtNodeMethods* SIBScalarToIntegerMethods;
extern const AtNodeMethods* SIBScalarsToVectorMethods;
extern const AtNodeMethods* SIBVectorToScalarMethods;
extern const AtNodeMethods* SIBSpaceConversionMethods;
extern const AtNodeMethods* Vector3ToVector2Methods;
extern const AtNodeMethods* Vector2ToVector3Methods;

// Math
extern const AtNodeMethods* SIBColorMathBasicMethods;
extern const AtNodeMethods* SIBColorMathExponentMethods;
extern const AtNodeMethods* SIBColorMathLogicMethods;
extern const AtNodeMethods* SIBColorMathUnaryMethods;
extern const AtNodeMethods* SIBColorSmoothRangeMethods;
extern const AtNodeMethods* MIBColorAverageMethods;
extern const AtNodeMethods* SIBInterpLinearMethods;
extern const AtNodeMethods* SIBColorInRangeMethods;
extern const AtNodeMethods* SIBScalarMathBasicMethods;
extern const AtNodeMethods* SIBScalarMathExponentMethods;
extern const AtNodeMethods* SIBScalarInRangeMethods;
extern const AtNodeMethods* SIBScalarMathUnaryMethods;
extern const AtNodeMethods* SIBScalarSmoothRangeMethods;
extern const AtNodeMethods* SIBScalarMathLogicMethods;
extern const AtNodeMethods* SIBBooleanMathLogicMethods;
extern const AtNodeMethods* SIBBooleanNegateMethods;
extern const AtNodeMethods* SIBVectorMathScalarMethods;
extern const AtNodeMethods* SIBVectorMathVectorMethods;
extern const AtNodeMethods* SIBAttenLinearMethods;
extern const AtNodeMethods* SIBScalarMathCurveMethods;
extern const AtNodeMethods* SIBColorMathCurveMethods;
extern const AtNodeMethods* Math_Int_FmodMethods;

// Share
extern const AtNodeMethods* SIBColorPassThroughMethods;
extern const AtNodeMethods* SIBScalarPassThroughMethods;
extern const AtNodeMethods* SIBIntegerPassThroughMethods;
extern const AtNodeMethods* SIBBooleanPassThroughMethods;
extern const AtNodeMethods* SIBVectorPassThroughMethods;

extern const AtNodeMethods* BooleanPassThroughMethods;
extern const AtNodeMethods* Color4PassThroughMethods;
extern const AtNodeMethods* IntegerPassThroughMethods;
extern const AtNodeMethods* MatrixPassThroughMethods;
extern const AtNodeMethods* ScalarPassThroughMethods;

// Array switchers
extern const AtNodeMethods* BooleanSwitchMethods;
extern const AtNodeMethods* Color4SwitchMethods;
extern const AtNodeMethods* IntegerSwitchMethods;
extern const AtNodeMethods* ScalarSwitchMethods;
extern const AtNodeMethods* Vector3SwitchMethods;

// Switch
extern const AtNodeMethods* SIBColorSwitchMethods;
extern const AtNodeMethods* SIBScalarSwitchMethods;
extern const AtNodeMethods* SIBVectorSwitchMethods;
extern const AtNodeMethods* SIBColorMultiSwitchMethods;
extern const AtNodeMethods* SIBScalarMultiSwitchMethods;
extern const AtNodeMethods* SIBVectorMultiSwitchMethods;

// Map LookUps
extern const AtNodeMethods* MIBTextureLookupMethods;
extern const AtNodeMethods* MapLookupColorMethods;
extern const AtNodeMethods* MapLookupScalarMethods;
extern const AtNodeMethods* MapLookupIntegerMethods;
extern const AtNodeMethods* MapLookupBooleanMethods;
extern const AtNodeMethods* MapLookupVectorMethods;
extern const AtNodeMethods* SIBVertexColorAlphaMethods;

// Texture Space Generators
extern const AtNodeMethods* SIBTexprojLookupMethods;

// Auxiliary Shaders
extern const AtNodeMethods* SIBImageClipMethods;
extern const AtNodeMethods* SIBMatteMethods;
extern const AtNodeMethods* SIBTextureLayerMethods;

////////////////////////////
// SItoA Shaders
////////////////////////////

extern const AtNodeMethods* PassthroughFilterMethods;
extern const AtNodeMethods* ClosureMethods;

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

enum SHADERS 
{
   // Texture
   MIB_TEXTURE_REMAP,
   TXT2D_IMAGE_EXPLICIT,
   TXT2D_SCALARIMAGE_EXPLICIT,
   TXT2D_GRADIENT_V2,
   TXT3D_CELLULAR_V3,
   TXT3D_CHECKERBOARD,
   TXT3D_MARBLE,
   SIB_TEXTURE_MARBLE,
   SIB_TEXTURE_SNOW,
   TXT3D_TEXTURE_SNOW,
   SIB_TEXTURE_GRID,
   TXT2D_TEXTURE_GRID,

   // Attributes
   SIB_ATTRIBUTE_BOOLEAN,
   SIB_ATTRIBUTE_COLOR,
   SIB_ATTRIBUTE_INTEGER,
   SIB_ATTRIBUTE_SCALAR,
   SIB_ATTRIBUTE_VECTOR,
   SIB_ATTRIBUTE_TRANSFORM,

   // Bump
   SIB_ZBUMP,

   // Color Channels
   SIB_CHANNEL_PICKER,
   SIB_COLOR_COMBINE,
   SIB_HLSA_COMBINE,
   SIB_HSVA_COMBINE,

   // Image Processing
   SIB_COLOR_CORRECTION,
   SIB_COLOR_BALANCE,
   SIB_COLOR_HLS_ADJUST,
   MIB_COLOR_INTENSITY,
   SIB_COLOR_INVERT,
   RGBA_KEYER,
   SIB_SCALAR_INVERT,

   // Mixers
   SIB_COLOR_2MIX,
   SIB_COLOR_8MIX,
   SIB_COLOR_GRADIENT,

   // Conversion
   SIB_COLOR_TO_SCALAR,
   SIB_COLOR_TO_VECTOR,
   SIB_COLOR_TO_BOOLEAN,
   SIB_COLOR_RGB_TO_HSV,
   SIB_COLOR_HSV_TO_RGB,
   SIB_INTEGER_TO_SCALAR,
   SIB_SCALAR_TO_COLOR,
   SIB_SCALAR_TO_INTEGER,
   SIB_SCALARS_TO_VECTOR,
   SIB_VECTOR_TO_SCALAR,
   SIB_SPACE_CONVERSION,
   VECTOR3TOVECTOR2, 
   VECTOR2TOVECTOR3, 

   // Math
   SIB_COLOR_MATH_BASIC,
   SIB_COLOR_MATH_EXPONENT,
   SIB_COLOR_MATH_LOGIC,
   SIB_COLOR_MATH_UNARY,
   SIB_COLOR_SMOOTHRANGE,
   MIB_COLOR_AVERAGE,  
   SIB_INTERP_LINEAR,
   SIB_COLOR_INRANGE,
   SIB_SCALAR_MATH_BASIC,
   SIB_SCALAR_MATH_EXPONENT,
   SIB_SCALAR_INRANGE,
   SIB_SCALAR_MATH_LOGIC,
   SIB_SCALAR_MATH_UNARY,
   SIB_SCALAR_SMOOTHRANGE,
   SIB_BOOLEAN_MATH_LOGIC,
   SIB_BOOLEAN_NEGATE,
   SIB_VECTOR_MATH_SCALAR,
   SIB_VECTOR_MATH_VECTOR,
   SIB_ATTEN_LINEAR,
   SIB_SCALAR_MATH_CURVE,
   SIB_COLOR_MATH_CURVE,
   MATH_INT_FMOD,

   // Share
   SIB_COLOR_PASSTHROUGH,
   SIB_COLOR_PASSTHROUGHT, // In XSI 7.01 is called with a T character at the end.
   SIB_SCALAR_PASSTHROUGH,
   SIB_INTEGER_PASSTHROUGH,
   SIB_BOOLEAN_PASSTHROUGH,
   SIB_VECTOR_PASSTHROUGH,

   BOOLEANPASSTHROUGH,
   COLOR4PASSTHROUGH,
   INTEGERPASSTHROUGH,
   MATRIXPASSTHROUGH,
   SCALARPASSTHROUGH,

   // Array switchers
   BOOLEAN_SWITCH,
   COLOR4_SWITCH,
   INTEGER_SWITCH,
   SCALAR_SWITCH,
   VECTOR3_SWITCH,

   // Switch
   SIB_COLOR_SWITCH,
   SIB_SCALAR_SWITCH,
   SIB_VECTOR_SWITCH,
   SIB_COLOR_MULTI_SWITCH,
   SIB_SCALAR_MULTI_SWITCH,
   SIB_VECTOR_MULTI_SWITCH,
   
   // Map LookUps
   MIB_TEXTURE_LOOKUP,
   MAP_LOOKUP_COLOR,
   MAP_LOOKUP_SCALAR,
   MAP_LOOKUP_INTEGER,
   MAP_LOOKUP_BOOLEAN,
   MAP_LOOKUP_VECTOR,
   SIB_VERTEX_COLOR_ALPHA,

   // Texture Space Generators
   SIB_TEXPROJ_LOOKUP,

   // Auxiliary Shaders
   SIB_IMAGE_CLIP,
   SIB_MATTE,
   SIB_TEXTURELAYER,
   
   PASSTHROUGHFILTER,
   CLOSURE
};

node_loader
{
   sprintf(node->version, AI_VERSION);

   switch (i) 
   {
      // Texture
      case MIB_TEXTURE_REMAP:
         node->methods      = mib_texture_remapMethods;
         node->output_type  = AI_TYPE_VECTOR;
         node->name         = "mib_texture_remap";
         node->node_type    = AI_NODE_SHADER;
         break;

      case TXT2D_IMAGE_EXPLICIT:
         node->methods      = txt2d_image_explicitMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "txt2d_image_explicit";
         node->node_type    = AI_NODE_SHADER;
      break;

      case TXT2D_SCALARIMAGE_EXPLICIT:
         node->methods      = txt2d_scalarimage_explicitMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "txt2d_scalarimage_explicit";
         node->node_type    = AI_NODE_SHADER;
      break;
   
      case TXT2D_GRADIENT_V2:
         node->methods      = txt2d_gradient_v2Methods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "txt2d_gradient_v2";
         node->node_type    = AI_NODE_SHADER;
      break;

      case TXT3D_CELLULAR_V3:
         node->methods      = txt3d_cellular_v3Methods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "txt3d_cellular_v3";
         node->node_type    = AI_NODE_SHADER;
         break;

      case TXT3D_CHECKERBOARD:
         node->methods      = txt3d_checkerboardMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "txt3d_checkerboard";
         node->node_type    = AI_NODE_SHADER;
         break;

      case TXT3D_MARBLE:
         node->methods      = txt3d_marbleMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "txt3d_marble";
         node->node_type    = AI_NODE_SHADER;
         break;
       
      case SIB_TEXTURE_MARBLE:
         node->methods      = sib_texture_marbleMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_texture_marble";
         node->node_type    = AI_NODE_SHADER;
         break;

      case SIB_TEXTURE_SNOW:
         node->methods      = SIBTextureSnowMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_texture_snow";
         node->node_type    = AI_NODE_SHADER;
         break;
         
      case TXT3D_TEXTURE_SNOW:
         node->methods      = TXT3DTextureSnowMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "txt3d_snow";
         node->node_type    = AI_NODE_SHADER;
         break;

      case SIB_TEXTURE_GRID:
         node->methods      = SIBTextureGridMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_texture2d_grid";
         node->node_type    = AI_NODE_SHADER;
         break;

      case TXT2D_TEXTURE_GRID:
         node->methods      = TXT2DTextureGridMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "txt2d_grid";
         node->node_type    = AI_NODE_SHADER;
         break;

      // Attributes
      case SIB_ATTRIBUTE_BOOLEAN:
         node->methods      = SIBAttributeBooleanMethods;
         node->output_type  = AI_TYPE_BOOLEAN;
         node->name         = "sib_attribute_boolean";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_ATTRIBUTE_COLOR:
         node->methods      = SIBAttributeColorMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_attribute_color";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_ATTRIBUTE_INTEGER:
         node->methods      = SIBAttributeIntegerMethods;
         node->output_type  = AI_TYPE_INT;
         node->name         = "sib_attribute_integer";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_ATTRIBUTE_SCALAR:
         node->methods      = SIBAttributeScalarMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_attribute_scalar";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_ATTRIBUTE_VECTOR:
         node->methods      = SIBAttributeVectorMethods;
         node->output_type  = AI_TYPE_VECTOR;
         node->name         = "sib_attribute_vector";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_ATTRIBUTE_TRANSFORM:
         node->methods      = SIBAttributeTransformMethods;
         node->output_type  = AI_TYPE_MATRIX;
         node->name         = "sib_attribute_transform";
         node->node_type    = AI_NODE_SHADER;
      break;

      // Bump
      case SIB_ZBUMP:
         node->methods      = SIBZBumpMethods;
         node->output_type  = AI_TYPE_VECTOR;
         node->name         = "sib_zbump";
         node->node_type    = AI_NODE_SHADER;
      break;

      // Color Channels
      case SIB_CHANNEL_PICKER:
         node->methods      = SIBChannelPickerMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_channel_picker";
         node->node_type    = AI_NODE_SHADER;
      break;
    
      case SIB_HLSA_COMBINE:
         node->methods      = SIBHLSACombineMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_hlsa_combine";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_HSVA_COMBINE:
         node->methods      = SIBHSVACombineMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_hsva_combine";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_COMBINE:
         node->methods      = SIBColorCombineMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_combine";
         node->node_type    = AI_NODE_SHADER;
      break;

      // Image Processing
      case SIB_COLOR_CORRECTION:
         node->methods      = SIBColorCorrectionMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_correction";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_BALANCE:
         node->methods      = SIBColorBalanceMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_balance";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_HLS_ADJUST:
         node->methods      = SIBColorHLSAdjustMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_hls_adjust";
         node->node_type    = AI_NODE_SHADER;
      break;
   
      case MIB_COLOR_INTENSITY:
         node->methods      = MIBColorIntensityMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "mib_color_intensity";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_INVERT:
         node->methods      = SIBColorInvertMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_invert";
         node->node_type    = AI_NODE_SHADER;
      break;

      case RGBA_KEYER:
         node->methods      = rgba_keyerMethods;
         node->output_type  = AI_TYPE_RGB;
         node->name         = "rgba_keyer";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SCALAR_INVERT:
         node->methods      = SIBScalarInvertMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_scalar_invert";
         node->node_type    = AI_NODE_SHADER;
      break;

      // Mixer
      case SIB_COLOR_2MIX:
         node->methods      = SIBColorMixMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_2mix";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_8MIX:
         node->methods      = SIBColorMixMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_8mix";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_GRADIENT:
         node->methods      = SIBColorGradientMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_gradient";
         node->node_type    = AI_NODE_SHADER;
      break;

      // Conversion
      case SIB_COLOR_TO_SCALAR:
         node->methods      = SIBColorToScalarMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_color_to_scalar";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_TO_VECTOR:
         node->methods      = SIBColorToVectorMethods;
         node->output_type  = AI_TYPE_VECTOR;
         node->name         = "sib_color_to_vector";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_TO_BOOLEAN:
         node->methods      = SIBColorToBooleanMethods;
         node->output_type  = AI_TYPE_BOOLEAN;
         node->name         = "sib_color_to_boolean";
         node->node_type    = AI_NODE_SHADER;
         break;

      case SIB_COLOR_RGB_TO_HSV:
         node->methods      = SIBColorRGBToHSVMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_rgb_to_hsv";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_HSV_TO_RGB:
         node->methods      = SIBColorHSVToRGBMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_hsv_to_rgb";
         node->node_type    = AI_NODE_SHADER;
      break;
      
      case SIB_INTEGER_TO_SCALAR:
         node->methods      = SIBIntegerToScalarMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_integer_to_scalar";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SCALAR_TO_COLOR:
         node->methods      = SIBScalarToColorMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_scalar_to_color";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SCALAR_TO_INTEGER:
         node->methods      = SIBScalarToIntegerMethods;
         node->output_type  = AI_TYPE_INT;
         node->name         = "sib_scalar_to_integer";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SCALARS_TO_VECTOR:
         node->methods      = SIBScalarsToVectorMethods;
         node->output_type  = AI_TYPE_VECTOR;
         node->name         = "sib_scalars_to_vector";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_VECTOR_TO_SCALAR:
         node->methods      = SIBVectorToScalarMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_vector_to_scalar";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SPACE_CONVERSION:
         node->methods      = SIBSpaceConversionMethods;
         node->output_type  = AI_TYPE_VECTOR;
         node->name         = "sib_space_conversion";
         node->node_type    = AI_NODE_SHADER;
         break;

      case VECTOR3TOVECTOR2:
         node->methods      = Vector3ToVector2Methods;
         node->output_type  = AI_TYPE_VECTOR2;
         node->name         = "Vector3ToVector2";
         node->node_type    = AI_NODE_SHADER;
         break;

      case VECTOR2TOVECTOR3:
         node->methods      = Vector2ToVector3Methods;
         node->output_type  = AI_TYPE_VECTOR;
         node->name         = "Vector2ToVector3";
         node->node_type    = AI_NODE_SHADER;
         break;

      // Math
      case SIB_COLOR_MATH_BASIC:
         node->methods      = SIBColorMathBasicMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_math_basic";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_MATH_EXPONENT:
         node->methods      = SIBColorMathExponentMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_math_exponent";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_MATH_LOGIC:
         node->methods      = SIBColorMathLogicMethods;
         node->output_type  = AI_TYPE_BOOLEAN;
         node->name         = "sib_color_math_logic";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_MATH_UNARY:
         node->methods      = SIBColorMathUnaryMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_math_unary";
         node->node_type    = AI_NODE_SHADER;
      break;
      
      case SIB_COLOR_SMOOTHRANGE:
         node->methods      = SIBColorSmoothRangeMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_smoothrange";
         node->node_type    = AI_NODE_SHADER;
      break;
      
      case MIB_COLOR_AVERAGE:
         node->methods      = MIBColorAverageMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "mib_color_average";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_INTERP_LINEAR:
         node->methods      = SIBInterpLinearMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_interp_linear";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_INRANGE:
         node->methods      = SIBColorInRangeMethods;
         node->output_type  = AI_TYPE_BOOLEAN;
         node->name         = "sib_color_inrange";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SCALAR_MATH_BASIC:
         node->methods      = SIBScalarMathBasicMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_scalar_math_basic";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SCALAR_MATH_EXPONENT:
         node->methods      = SIBScalarMathExponentMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_scalar_math_exponent";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SCALAR_INRANGE:
         node->methods      = SIBScalarInRangeMethods;
         node->output_type  = AI_TYPE_BOOLEAN;
         node->name         = "sib_scalar_inrange";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SCALAR_MATH_LOGIC:
         node->methods      = SIBScalarMathLogicMethods;
         node->output_type  = AI_TYPE_BOOLEAN;
         node->name         = "sib_scalar_math_logic";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SCALAR_MATH_UNARY:
         node->methods      = SIBScalarMathUnaryMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_scalar_math_unary";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SCALAR_SMOOTHRANGE:
         node->methods      = SIBScalarSmoothRangeMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_scalar_smoothrange";
         node->node_type    = AI_NODE_SHADER;
      break;
 
      case SIB_BOOLEAN_MATH_LOGIC:
         node->methods      = SIBBooleanMathLogicMethods;
         node->output_type  = AI_TYPE_BOOLEAN;
         node->name         = "sib_boolean_math_logic";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_BOOLEAN_NEGATE:
         node->methods      = SIBBooleanNegateMethods;
         node->output_type  = AI_TYPE_BOOLEAN;
         node->name         = "sib_boolean_negate";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_VECTOR_MATH_SCALAR:
         node->methods      = SIBVectorMathScalarMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_vector_math_scalar";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_VECTOR_MATH_VECTOR:
         node->methods      = SIBVectorMathVectorMethods;
         node->output_type  = AI_TYPE_VECTOR;
         node->name         = "sib_vector_math_vector";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_ATTEN_LINEAR:
         node->methods      = SIBAttenLinearMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_atten_linear";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SCALAR_MATH_CURVE:
         node->methods      = SIBScalarMathCurveMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_scalar_math_curve";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_MATH_CURVE:
         node->methods      = SIBColorMathCurveMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_math_curve";
         node->node_type    = AI_NODE_SHADER;
      break;

      case MATH_INT_FMOD:
         node->methods      = Math_Int_FmodMethods;
         node->output_type  = AI_TYPE_INT;
         node->name         = "Math_int_fmod";
         node->node_type    = AI_NODE_SHADER;
      break;

      // Share
      case SIB_COLOR_PASSTHROUGH:
         node->methods      = SIBColorPassThroughMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_passthrough";
         node->node_type    = AI_NODE_SHADER;
      break;                     

      // In XSI 7.01 Color Shared has a T character at the end of 
      // the node name
      case SIB_COLOR_PASSTHROUGHT:   
         node->methods      = SIBColorPassThroughMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_passthrought";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SCALAR_PASSTHROUGH:
         node->methods      = SIBScalarPassThroughMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_scalar_passthrough";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_INTEGER_PASSTHROUGH:
         node->methods      = SIBIntegerPassThroughMethods;
         node->output_type  = AI_TYPE_INT;
         node->name         = "sib_integer_passthrough";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_BOOLEAN_PASSTHROUGH:
         node->methods      = SIBBooleanPassThroughMethods;
         node->output_type  = AI_TYPE_BOOLEAN;
         node->name         = "sib_boolean_passthrough";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_VECTOR_PASSTHROUGH:
         node->methods      = SIBVectorPassThroughMethods;
         node->output_type  = AI_TYPE_VECTOR;
         node->name         = "sib_vector_passthrough";
         node->node_type    = AI_NODE_SHADER;
      break;

      case BOOLEANPASSTHROUGH:
         node->methods      = BooleanPassThroughMethods;
         node->output_type  = AI_TYPE_BOOLEAN;
         node->name         = "BooleanPassthrough";
         node->node_type    = AI_NODE_SHADER;
      break;

      case COLOR4PASSTHROUGH:
         node->methods      = Color4PassThroughMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "Color4Passthrough";
         node->node_type    = AI_NODE_SHADER;
      break;

      case INTEGERPASSTHROUGH:
         node->methods      = IntegerPassThroughMethods;
         node->output_type  = AI_TYPE_INT;
         node->name         = "IntegerPassthrough";
         node->node_type    = AI_NODE_SHADER;
      break;

      case MATRIXPASSTHROUGH:
         node->methods      = MatrixPassThroughMethods;
         node->output_type  = AI_TYPE_MATRIX;
         node->name         = "MatrixPassthrough";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SCALARPASSTHROUGH:
         node->methods      = ScalarPassThroughMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "ScalarPassthrough";
         node->node_type    = AI_NODE_SHADER;
      break;

     // Array switchers
   
      case BOOLEAN_SWITCH:
         node->methods      = BooleanSwitchMethods;
         node->output_type  = AI_TYPE_BOOLEAN;
         node->name         = "BooleanSwitch";
         node->node_type    = AI_NODE_SHADER;
      break;

      case COLOR4_SWITCH:
         node->methods      = Color4SwitchMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "Color4Switch";
         node->node_type    = AI_NODE_SHADER;
      break;

      case INTEGER_SWITCH:
         node->methods      = IntegerSwitchMethods;
         node->output_type  = AI_TYPE_INT;
         node->name         = "IntegerSwitch";
         node->node_type    = AI_NODE_SHADER;
      break;

      case VECTOR3_SWITCH:
         node->methods      = Vector3SwitchMethods;
         node->output_type  = AI_TYPE_VECTOR;
         node->name         = "Vector3Switch";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SCALAR_SWITCH:
         node->methods      = ScalarSwitchMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "ScalarSwitch";
         node->node_type    = AI_NODE_SHADER;
      break;

      // Switch
      case SIB_COLOR_SWITCH:
         node->methods      = SIBColorSwitchMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_switch";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SCALAR_SWITCH:
         node->methods      = SIBScalarSwitchMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_scalar_switch";
         node->node_type    = AI_NODE_SHADER;
      break;
      
      case SIB_VECTOR_SWITCH:
         node->methods      = SIBVectorSwitchMethods;
         node->output_type  = AI_TYPE_VECTOR;
         node->name         = "sib_vector_switch";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_COLOR_MULTI_SWITCH:
         node->methods      = SIBColorMultiSwitchMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_color_multi_switch";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_SCALAR_MULTI_SWITCH:
         node->methods      = SIBScalarMultiSwitchMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "sib_scalar_multi_switch";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_VECTOR_MULTI_SWITCH:
         node->methods      = SIBVectorMultiSwitchMethods;
         node->output_type  = AI_TYPE_VECTOR;
         node->name         = "sib_vector_multi_switch";
         node->node_type    = AI_NODE_SHADER;
      break;

      // Map LookUp
      case MIB_TEXTURE_LOOKUP:
         node->methods      = MIBTextureLookupMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "mib_texture_lookup";
         node->node_type    = AI_NODE_SHADER;
      break;

      case MAP_LOOKUP_COLOR:
         node->methods      = MapLookupColorMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "map_lookup_color";
         node->node_type    = AI_NODE_SHADER;
      break;

      case MAP_LOOKUP_SCALAR:
         node->methods      = MapLookupScalarMethods;
         node->output_type  = AI_TYPE_FLOAT;
         node->name         = "map_lookup_scalar";
         node->node_type    = AI_NODE_SHADER;
      break;

      case MAP_LOOKUP_INTEGER:
         node->methods      = MapLookupIntegerMethods;
         node->output_type  = AI_TYPE_INT;
         node->name         = "map_lookup_integer";
         node->node_type    = AI_NODE_SHADER;
      break;

      case MAP_LOOKUP_BOOLEAN:
         node->methods      = MapLookupBooleanMethods;
         node->output_type  = AI_TYPE_BOOLEAN;
         node->name         = "map_lookup_boolean";
         node->node_type    = AI_NODE_SHADER;
      break;

      case MAP_LOOKUP_VECTOR:
         node->methods      = MapLookupVectorMethods;
         node->output_type  = AI_TYPE_VECTOR;
         node->name         = "map_lookup_vector";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_VERTEX_COLOR_ALPHA:
         node->methods      = SIBVertexColorAlphaMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_vertex_color_alpha";
         node->node_type    = AI_NODE_SHADER;
      break;

      // Texture Space Generators
      case SIB_TEXPROJ_LOOKUP:
         node->methods      = SIBTexprojLookupMethods;
         node->output_type  = AI_TYPE_VECTOR;
         node->name         = "sib_texproj_lookup";
         node->node_type    = AI_NODE_SHADER;
      break;
      
      // Auxiliary Shaders
      case SIB_IMAGE_CLIP:
         node->methods      = SIBImageClipMethods;
         node->output_type  = AI_TYPE_RGBA;
         node->name         = "sib_image_clip";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_MATTE:
         node->methods      = SIBMatteMethods;
         node->output_type  = AI_TYPE_RGB;
         node->name         = "sib_matte";
         node->node_type    = AI_NODE_SHADER;
      break;

      case SIB_TEXTURELAYER:
         node->methods      = SIBTextureLayerMethods;
         node->output_type  = AI_TYPE_RGB;
         node->name         = "sib_texturelayer";
         node->node_type    = AI_NODE_SHADER;
      break;
 
      // SItoA Shaders
      case PASSTHROUGHFILTER:
         node->methods      = PassthroughFilterMethods;
         node->output_type  = AI_TYPE_RGB;
         node->name         = "passthrough_filter";
         node->node_type    = AI_NODE_SHADER;
         break;

      case CLOSURE:
         node->methods      = ClosureMethods;
         node->output_type  = AI_TYPE_CLOSURE;
         node->name         = "closure";
         node->node_type    = AI_NODE_SHADER;
         break;

      default:
         return false;
   }

   return true;
}
