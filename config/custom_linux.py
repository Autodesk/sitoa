# Sample configuration file for Linux
#
# Copy this file as 'custom.py' at the root of the repo if you need to customize
# the build variables

SHCXX       = r'/usr/bin/gcc-4.2.4/bin/gcc-4.2.4'

XSISDK_ROOT = r'/usr/Softimage/Softimage_2015/XSISDK'
ARNOLD_HOME = r'/usr/SolidAngle/Arnold-6.1.0.1/linux'

TARGET_WORKGROUP_PATH = './dist/Softimage_2015/Addons/SItoA'

WARN_LEVEL  = 'warn-only' # lots of warnings are expected on Linux
MODE        = 'opt'
SHOW_CMDS   = True
