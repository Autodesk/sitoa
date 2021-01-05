# Sample configuration file for Windows
#
# Copy this file as 'custom.py' at the root of the repo if you need to customize
# the build variables

TARGET_ARCH         = 'x86_64'
MSVC_VERSION        = '14.0'
VS_HOME             = r'C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC'
WINDOWS_KIT         = r'C:/Program Files (x86)/Windows Kits/8.0'

XSISDK_ROOT         = r'C:/Program Files/Autodesk/Softimage 2015/XSISDK'
ARNOLD_HOME         = r'C:/SolidAngle/Arnold-6.1.0.1/win64'

TARGET_WORKGROUP_PATH  = r'./Softimage_2015/Addons/SItoA'

WARN_LEVEL = 'strict'
MODE       = 'opt'
SHOW_CMDS  = True

PATCH_ADLM = True
