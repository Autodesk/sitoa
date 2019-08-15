# vim: filetype=python

import sys, os

## First we extend the module path to load our own modules.
## This path has to be absolute, otherwise, nested scripts could not find
## our python modules
sys.path = [os.path.abspath(os.path.join('tools', 'python'))] + sys.path

import system
from build_tools import *

import glob, shutil


import SCons


## Creates a release package for SItoA
def make_package(target, source, env):
   package_name = str(target[0]) + ".xsiaddon"
   zip_name = str(target[0])
   base_pkg_dir = os.path.join('dist', 'package_temp' + get_softimage_version(env['XSISDK_ROOT']));
   
   # First we make sure the temp directory doesn't exist
   #if os.path.exists(base_pkg_dir):
   #   shutil.rmtree(base_pkg_dir)

   # Copy all package files into the temporary directory   
   for t in PACKAGE_FILES:
      target_dir = os.path.join(base_pkg_dir, str(t[1]))
      file_spec = str(t[0])
      if os.path.isdir(file_spec):
         # If the first element is a directory, use copytree to make a recursive copy
         copy_dir_recursive(file_spec, target_dir)
      else:
         # If the first element is a file or wildcard, create the target dir and copy files separately
         if not os.path.exists(target_dir):
            os.makedirs(target_dir)
         file_list = glob.glob(file_spec)
      
         # Optionally rename the destination file (only for single files)
         if (len(file_list) == 1) and (len(t) == 3):
            shutil.copy2(file_list[0], os.path.join(target_dir, str(t[2])))
         else:
            for f in file_list:
               shutil.copy2(f, target_dir)
   
   # Now we generate deploy scripts
   f = open(os.path.join(base_pkg_dir, 'deploy_sitoa.js'), 'w')
   f.write('''
function main()
{
   LoadPreset("%s", "");
   PackageAddon("%s", "%s", true, "src;Debug;Ship");
}
''' % (os.path.abspath(os.path.join(base_pkg_dir, 'deploy_sitoa.Preset')).replace("\\", "\\\\"),
       os.path.abspath(os.path.join(base_pkg_dir, 'Addons', 'SItoA')).replace("\\", "\\\\"),
       os.path.abspath(base_pkg_dir).replace("\\", "\\\\")))
   f.close()

   saferemove(package_name)

   BINPATH = os.path.join(env['XSISDK_ROOT'], '..', 'Application', 'bin')

   command_string = '%s -script "%s" -main main -args -inpath "%s" -outpath "%s" -preset "%s" -processing' % (
      '"' + os.path.join(BINPATH, 'xsibatch') + '"',
      os.path.join(base_pkg_dir, 'deploy_sitoa.js'),
      os.path.join(base_pkg_dir, 'Addons', 'SItoA'),
      base_pkg_dir,
      os.path.join(base_pkg_dir, 'deploy_sitoa.Preset'))
      
   if command_string != '':
      p = subprocess.Popen(command_string, shell=(system.os() == 'linux'), stdout = None)
      # Commented... is returning always != 0 ?
      retcode = p.wait()
      #if retcode != 0:
      #   print "ERROR: Could not create package '%s'" % package_name
      #else:
      shutil.move(os.path.join(base_pkg_dir, 'SItoA.xsiaddon'), os.path.join('dist', package_name))
   """
   import zipfile

   zp = zipfile.ZipFile('%s.zip' % zip_name , 'w', zipfile.ZIP_DEFLATED)
   zp.write(package_name)

   # Clean temporary .addon file
   saferemove(package_name)
   
   # Clean temporary directory
   #shutil.rmtree(base_pkg_dir)
   """
   return None
   
if system.os() == 'windows':
   ALLOWED_COMPILERS = ['msvc', 'icc']
else:
   ALLOWED_COMPILERS = ['gcc']

# load custom or default build variables
if os.path.isfile('custom.py'):
   vars = Variables('custom.py')
else:
   vars = Variables('custom_%s.py' % system.os())
    
vars.AddVariables(
      ## basic options
      EnumVariable('MODE',        'Set compiler configuration', 'debug', allowed_values=('opt', 'debug', 'profile')),
      EnumVariable('WARN_LEVEL',  'Set warning level',          'strict', allowed_values=('strict', 'warn-only', 'none')),
      EnumVariable('COMPILER',    'Set compiler to use',        ALLOWED_COMPILERS[0], allowed_values=ALLOWED_COMPILERS),
      EnumVariable('TARGET_ARCH', 'Allows compiling for a different architecture', system.host_arch(), allowed_values=system.get_valid_target_archs()),
      BoolVariable('SHOW_CMDS',   'Display the actual command lines used for building', False),

      EnumVariable('SHOW_TEST_OUTPUT', 'Display the test log as it is being run', 'single', allowed_values=('always', 'never', 'single') ),
      BoolVariable('UPDATE_REFERENCE', 'Update the reference log/image for the specified targets', False),
      ('TEST_PATTERN' , 'Glob pattern of tests to be run', 'test_*'),

      PathVariable('XSISDK_ROOT', 'Where to find XSI libraries', get_default_path('XSISDK_ROOT', '.')),
      PathVariable('ARNOLD_HOME', 'Base Arnold dir', '.'),
      PathVariable('VS_HOME', 'Visual Studio 11 home', '.'),
      PathVariable('WINDOWS_KIT', 'Windows Kit home', '.'),

      PathVariable('TARGET_WORKGROUP_PATH', 'Path used for installation of plugins', '.', PathVariable.PathIsDirCreate),
      PathVariable('SHCXX', 'C++ compiler used for generating shared-library objects', None),
      ('FTP'            , 'Path of the FTP to upload the package',        ''),
      ('FTP_SUBDIR'     , 'Subdirectory on the FTP to place the package' , ''),
      ('FTP_USER'       , 'Username for the FTP'                         , ''),
      ('FTP_PASS'       , 'Password for the FTP'                         , ''),
      ('PACKAGE_SUFFIX' , 'Suffix for the package names'                 , ''),
)

        
if system.os() == 'windows':
   vars.Add(EnumVariable('MSVC_VERSION', 'Version of MS Visual Studio to use', '9.0', allowed_values=('8.0', '8.0Exp', '9.0', '9.0Exp', '10.0', '11.0')))

env = Environment(variables = vars)

system.set_target_arch(env['TARGET_ARCH'])

ARNOLD_HOME = env['ARNOLD_HOME']
ARNOLD_API_INCLUDES = os.path.join(ARNOLD_HOME, 'include')
ARNOLD_BINARIES = os.path.join(ARNOLD_HOME, 'bin')
ARNOLD_PLUGINS = os.path.join(ARNOLD_HOME, 'plugins')
if system.os() == 'windows':
  ARNOLD_API_LIB = os.path.join(ARNOLD_HOME, 'lib')
else:
  ARNOLD_API_LIB = ARNOLD_BINARIES

VS_HOME = env['VS_HOME']
WINDOWS_KIT  = env['WINDOWS_KIT']

# Find XSISDK_VERSION by parsing xsi_version.h in the SDK
XSISDK_VERSION = get_softimage_version(env['XSISDK_ROOT']);

PACKAGE_SUFFIX = env.subst(env['PACKAGE_SUFFIX'])

print ""
print "Building      : SItoA %s" % (get_sitoa_version(os.path.join('plugins', 'sitoa', 'version.cpp')))
print "Arnold SDK    : %s" % get_arnold_version(os.path.join(ARNOLD_API_INCLUDES, 'ai_version.h'))
print "Softimage SDK : %s" % XSISDK_VERSION     
print "Mode          : %s" % (env['MODE'])
print "Host OS       : %s" % (system.os())
print "SCons         : %s" % (SCons.__version__)
print ""

################################
## COMPILER OPTIONS
################################

if system.os() == 'darwin':
   env.Append(CPPDEFINES = Split('_DARWIN'))
elif system.os() == 'linux':
   env.Append(CPPDEFINES = Split('_LINUX'))
elif system.os() == 'windows':
   # Embed manifest in executables and dynamic libraries
   env['LINKCOM'] = [env['LINKCOM'], 'mt.exe -nologo -manifest ${TARGET}.manifest -outputresource:$TARGET;1']
   env['SHLINKCOM'] = [env['SHLINKCOM'], 'mt.exe -nologo -manifest ${TARGET}.manifest -outputresource:$TARGET;2']
   ## Platform related defines
   env.Append(CPPDEFINES = Split('_WINDOWS _WIN32 WIN32'))
   env.Append(CPPDEFINES = Split('_WIN64'))

if env['COMPILER'] == 'gcc':
   ## warning level
   if env['WARN_LEVEL'] == 'none':
      env.Append(CCFLAGS = Split('-w'))
   else:
      env.Append(CCFLAGS = Split('-Wall -Wsign-compare'))
      if env['WARN_LEVEL'] == 'strict':
         env.Append(CCFLAGS = Split('-Werror'))

   ## optimization flags
   if env['MODE'] == 'opt' or env['MODE'] == 'profile':
      env.Append(CCFLAGS = Split('-O3'))

   if env['MODE'] == 'debug' or env['MODE'] == 'profile':
      if system.os() == 'darwin': 
         env.Append(CCFLAGS = Split('-gstabs')) 
         env.Append(LINKFLAGS = Split('-gstabs')) 
      else: 
         env.Append(CCFLAGS = Split('-g')) 
         env.Append(LINKFLAGS = Split('-g')) 

   if system.os() == 'darwin':
      if system.target_arch() == 'x86_64':
         ## tell gcc to compile a 64 bit binary
         env.Append(CCFLAGS = Split('-arch x86_64'))
         env.Append(LINKFLAGS = Split('-arch x86_64'))
      else:
         ## tell gcc to compile a 32 bit binary
         env.Append(CCFLAGS = Split('-arch i386'))
         env.Append(LINKFLAGS = Split('-arch i386'))

elif env['COMPILER'] == 'msvc':
   env.Append(CCFLAGS = Split('/EHsc /Gd /fp:precise'))
   env.Append(LINKFLAGS = Split('/MANIFEST'))
   if env['MODE'] == 'opt':
      env.Append(CCFLAGS = Split('/Ob2 /MD /O2'))
      env.Append(CPPDEFINES = Split('NDEBUG'))
   elif env['MODE'] == 'profile':
      env.Append(CCFLAGS = Split('/Ob2 /MD /Zi'))
   else:  ## Debug mode
      env.Append(CCFLAGS = Split('/Od /Zi /MDd'))
      env.Append(LINKFLAGS = Split('/DEBUG'))
      env.Append(CPPDEFINES = Split('_DEBUG'))
      
elif env['COMPILER'] == 'icc':
   if system.target_arch() == 'x86_64':
      env.Tool('intelc', abi = 'intel64')
   else:
      env.Tool('intelc', abi = 'x86')
   
   env['LINK']  = 'xilink'
   
   env.Append(CCFLAGS = Split('/EHsc /GS /GR /Qprec /Qvec-report0 /Qwd1478 /Qwd1786 /Qwd537 /Qwd1572 /Qwd991 /Qwd424'))
   if system.target_arch() == 'x86_64':
      pass
      # Enable this for 64 bit portability warnings
      ##env.Append(CCFLAGS = Split('/Wp64'))  
   else:
      env.Append(CCFLAGS = Split('/Gd'))
   
   if env['MODE'] == 'opt':
      env.Append(CCFLAGS = Split('/Ob2 /MD -G7 -O3 -QaxW -Qipo'))
      env.Append(LINKFLAGS = Split('/INCREMENTAL:NO'))
      env.Append(CPPDEFINES = Split('NDEBUG'))
   elif env['MODE'] == 'profile':
      env.Append(CCFLAGS = Split('/Ob2 /MD -G7 -O3 -QaxW -Qipo /Zi'))
   else:  ## Debug mode
      env.Append(CCFLAGS = Split('/Od /Zi /MDd'))
      env.Append(LINKFLAGS = Split('/DEBUG /INCREMENTAL'))
      env.Append(CPPDEFINES = Split('_DEBUG'))

env.Append(CPPPATH = ARNOLD_API_INCLUDES)
if system.os() == 'windows':
  env.Append(CPPPATH = [os.path.join(VS_HOME, 'include')])
  env.Append(CPPPATH = [os.path.join(WINDOWS_KIT, 'Include', 'um')])
  env.Append(CPPPATH = [os.path.join(WINDOWS_KIT, 'Include', 'shared')])

env.Append(LIBPATH = ARNOLD_API_LIB)
if system.os() == 'windows':
  env.Append(LIBPATH = [os.path.join(WINDOWS_KIT, 'Lib' , 'win8', 'um', 'x64')])
  env.Append(LIBPATH = [os.path.join(VS_HOME, 'lib', 'amd64')])

## configure base directory for temp files                                                   
BUILD_BASE_DIR = os.path.join('build', '%s_%s' % (system.os(), system.target_arch()), '%s_%s' % (env['COMPILER'], env['MODE']), 'si_%s' % XSISDK_VERSION)

if not env['SHOW_CMDS']:
   ## hide long compile lines from the user
   env['CCCOMSTR']     = 'Compiling $SOURCE ...'
   env['SHCCCOMSTR']   = 'Compiling $SOURCE ...'
   env['CXXCOMSTR']    = 'Compiling $SOURCE ...'
   env['SHCXXCOMSTR']  = 'Compiling $SOURCE ...'
   env['LINKCOMSTR']   = 'Linking $TARGET ...'
   env['SHLINKCOMSTR'] = 'Linking $TARGET ...'

################################
## BUILD TARGETS
################################

env['BUILDERS']['MakePackage'] = Builder(action = Action(make_package, "Preparing release package: '$TARGET'"))
env['ROOT_DIR'] = os.getcwd()

# r3d remove!
if system.os() == 'linux':
   add_to_library_path(env, '.')
   if os.environ.has_key('LD_LIBRARY_PATH'):
      add_to_library_path(env, os.environ['LD_LIBRARY_PATH'])
   os.environ['LD_LIBRARY_PATH'] = env['ENV']['LD_LIBRARY_PATH']
elif system.os() == 'windows':
   add_to_library_path(env, os.environ['PATH'])
   os.environ['PATH'] = env['ENV']['PATH']
                    
if system.os() == 'windows':
   [SITOA, SITOA_PRJ] = env.SConscript(os.path.join('plugins', 'sitoa', 'SConscript'),
                                       variant_dir = os.path.join(BUILD_BASE_DIR, 'sitoa'),
                                       duplicate = 0,
                                       exports   = 'env')
   
   [SITOA_SHADERS, SITOA_SHADERS_PRJ] = env.SConscript(os.path.join('shaders', 'src', 'SConscript'),
                                                       variant_dir = os.path.join(BUILD_BASE_DIR, 'shaders'),
                                                       duplicate = 0,
                                                       exports   = 'env')
   
   INSTALL_PRJ = env.MSVSProject(target = 'install' + env['MSVS']['PROJECTSUFFIX'],
                                 srcs = [],
                                 incs = [],
                                 buildtarget = 'install',
                                 cmdargs = ['-Q -s COMPILER=msvc MODE=debug TARGET_ARCH=x86',
                                            '-Q -s COMPILER=icc MODE=debug TARGET_ARCH=x86',
                                            '-Q -s COMPILER=msvc MODE=opt TARGET_ARCH=x86',
                                            '-Q -s COMPILER=icc MODE=opt TARGET_ARCH=x86'],
                                 variant = ['Debug_MSVC|Win32',
                                            'Debug_ICC|Win32',
                                            'Opt_MSVC|Win32',
                                            'Opt_ICC|Win32'],
                                 auto_build_solution = 0,
                                 nokeep = 1)

   SOLUTION = env.MSVSSolution(target = 'sitoa' + env['MSVS']['SOLUTIONSUFFIX'],
                               projects = [os.path.join('plugins', 'sitoa', 'sitoa') + env['MSVS']['PROJECTSUFFIX'],
                                           os.path.join('shaders', 'src', 'sitoa_shaders') + env['MSVS']['PROJECTSUFFIX'],
                                           'install' + env['MSVS']['PROJECTSUFFIX']],  ## TODO: Find a clean way of getting these project paths
                               dependencies = [[], [], ['sitoa', 'sitoa_shaders']],
                               variant = ['Debug_MSVC|Win32',
                                          'Debug_ICC|Win32',
                                          'Opt_MSVC|Win32',
                                          'Opt_ICC|Win32'])
else:
   SITOA = env.SConscript(os.path.join('plugins', 'sitoa', 'SConscript'),
                          variant_dir = os.path.join(BUILD_BASE_DIR, 'sitoa'),
                          duplicate = 0,
                          exports   = 'env')

   SITOA_SHADERS = env.SConscript(os.path.join('shaders', 'src', 'SConscript'),
                                  variant_dir = os.path.join(BUILD_BASE_DIR, 'shaders'),
                                  duplicate = 0,
                                  exports   = 'env')

SConscriptChdir(0)
TESTSUITE = env.SConscript(os.path.join('testsuite', 'SConscript'),
                           variant_dir = os.path.join(BUILD_BASE_DIR, 'testsuite'),
                           duplicate = 0,
                           exports   = 'env BUILD_BASE_DIR SITOA SITOA_SHADERS')
SConscriptChdir(1)

# hack, needs to be updated when the new versions of Softimage come o:)
try:
   SOFTIMAGE_VERSION = {10000: "2012", 11000: "2013", 12000: "2014", 13000 : "2015"}[int(XSISDK_VERSION)]
except:
   SOFTIMAGE_VERSION = XSISDK_VERSION
   
## Sets release package name based on Arnold version, architecture and compiler used.
package_name = "SItoA-" + get_sitoa_version(os.path.join('plugins', 'sitoa', 'version.cpp')) + "-" + system.os() + "-" + SOFTIMAGE_VERSION + PACKAGE_SUFFIX
PACKAGE = env.MakePackage(package_name, SITOA + SITOA_SHADERS)

## Specifies the files that will be included in the release package.
## List items have 2 or 3 elements, with 3 possible formats:
##
## (source_file, destination_path [, new_file_name])    Copies file to destination path, optionally renaming it
## (source_dir, destination_dir)                        Recursively copies the source directory as destination_dir
## (file_spec, destination_path)                        Copies a group of files specified by a glob expression
##
addon_path = os.path.join('Addons', 'SItoA')
plugins_path = os.path.join('Application', 'Plugins')
pictures_path = os.path.join('Application', 'Plugins', 'Pictures')
bin_path = os.path.join(plugins_path, 'bin', 'nt-x86-64')

if system.os() != 'windows':
   bin_path = os.path.join(plugins_path, 'bin', 'linux', 'x64')

license_path = os.path.join(bin_path, 'license')
pit_path = os.path.join(license_path, 'pit')

DLLS = '*.dll'   
plugin_binary_path = ""

# This explicit handling doesn't support other compilers like ICC
if system.os() == 'windows':
   if env['MODE'] == 'opt':
      plugin_binary_path = os.path.join('build', 'windows_x86_64', 'msvc_opt', "si_%s" % XSISDK_VERSION)
   else:
      plugin_binary_path = os.path.join('build', 'windows_x86_64', 'msvc_debug', "si_%s" % XSISDK_VERSION)
else:
   if env['MODE'] == 'opt':
      plugin_binary_path = os.path.join('build', 'linux_x86_64', 'gcc_opt', "si_%s" % XSISDK_VERSION)
   else:
      plugin_binary_path = os.path.join('build', 'linux_x86_64', 'gcc_debug', "si_%s" % XSISDK_VERSION)   
   DLLS = '*.so'

PACKAGE_FILES = [
[os.path.join(plugin_binary_path, 'sitoa', DLLS),                          os.path.join(addon_path, bin_path)],
[os.path.join(plugin_binary_path, 'shaders', DLLS),                        os.path.join(addon_path, bin_path)],
[os.path.join(ARNOLD_BINARIES, 'kick%s' % get_executable_extension()),     os.path.join(addon_path, bin_path)],
[os.path.join(ARNOLD_BINARIES, 'maketx%s' % get_executable_extension()),   os.path.join(addon_path, bin_path)],
[os.path.join(ARNOLD_BINARIES, 'noice%s' % get_executable_extension()),    os.path.join(addon_path, bin_path)],
[os.path.join(ARNOLD_BINARIES, 'oslc%s' % get_executable_extension()),     os.path.join(addon_path, bin_path)],
[os.path.join(ARNOLD_BINARIES, 'oslinfo%s' % get_executable_extension()),  os.path.join(addon_path, bin_path)],
[os.path.join(ARNOLD_BINARIES, '*%s' % get_library_extension()),           os.path.join(addon_path, bin_path)],
[os.path.join(ARNOLD_BINARIES, '*%s.*' % get_library_extension()),         os.path.join(addon_path, bin_path)],
[os.path.join(ARNOLD_BINARIES, '*.pit'),                                   os.path.join(addon_path, bin_path)],
[os.path.join(ARNOLD_PLUGINS, '*'),                                        os.path.join(addon_path, bin_path, '..', 'plugins')],
[os.path.join('plugins', 'helpers', '*.js'),                               os.path.join(addon_path, plugins_path)],
[os.path.join('plugins', 'helpers', '*.py'),                               os.path.join(addon_path, plugins_path)],
[os.path.join('plugins', 'helpers', 'Pictures', '*.bmp'),                  os.path.join(addon_path, pictures_path)],
[os.path.join('shaders', 'metadata', '*.mtd'),                             os.path.join(addon_path, bin_path)],
[os.path.join('plugins', 'metadata', '*.mtd'),                             os.path.join(addon_path, bin_path)],
[os.path.join(ARNOLD_HOME, 'license', 'lmuti*'),                           os.path.join(addon_path, license_path)],
[os.path.join(ARNOLD_HOME, 'license', 'rl*'),                              os.path.join(addon_path, license_path)],
[os.path.join(ARNOLD_HOME, 'license', 'solidangle.*'),                     os.path.join(addon_path, license_path)],
[os.path.join(ARNOLD_HOME, 'license', 'pit', '*'),                         os.path.join(addon_path, pit_path)]
]

import ftplib

def deploy(target, source, env):

   def ftp_send_binary_cb(block):
      print "\b#",
   
   package_name = str(source[0])
   package_name += '.zip'
   
   server = env['FTP']

   ftp = ftplib.FTP(server)

   ftp.login(env['FTP_USER'], env['FTP_PASS'])

   directory = env['FTP_SUBDIR']

   directory_split = directory.split('/')

   for d in directory_split:
      try:
         ftp.cwd(d)
      except:
         ftp.mkd(d)
         ftp.cwd(d)

   f = open(os.path.abspath(package_name), 'rb')
   print 'Sending "%s" to %s/%s...' % (source[0], server, directory)
   command = "STOR %s" % package_name
   try:
      ftp.storbinary(command, f, 81920, ftp_send_binary_cb)
   except:
      # Old python versions have no ftp callback
      ftp.storbinary(command, f, 81920)
   print

   f.close()
   ftp.close()  

env['BUILDERS']['PackageDeploy']  = Builder(action = Action(deploy,  "Deploying release package: '$SOURCE'"))

DEPLOY = env.PackageDeploy('deploy', package_name)

################################
## PATCH ADLM
################################

def patch_adlm(target, source, env):
   new_adlmint_last_char = '2'  # ONLY ONE CHARACTER
   if system.os() == 'windows':
      adclmhub_name = 'AdClmHub_1.dll'
      adlmint_name = 'adlmint.dll'
      size = 383280
      seek_pos = 266236
   else:
      adclmhub_name = 'libAdClmHub.so'
      adlmint_name = 'libadlmint.so'
      size = 1853576
      seek_pos = 779034

   new_adlmint_name = os.path.splitext(adlmint_name)[0][:-1] + new_adlmint_last_char + get_library_extension()

   wg_bin_path = os.path.normpath(os.path.join(env['TARGET_WORKGROUP_PATH'], bin_path))
   adclmhub_path = os.path.join(wg_bin_path, adclmhub_name)
   adlmint_path = os.path.join(wg_bin_path, adlmint_name)
   new_adlmint_path = os.path.join(wg_bin_path, new_adlmint_name)

   need_to_patch = False

   if os.path.isfile(adclmhub_path):
      # check file size as a way to see if patching is needed
      if os.path.getsize(adclmhub_path) == size:
         need_to_patch = True
      
   if not os.path.isfile(adlmint_path):
      need_to_patch = False

   if need_to_patch:
      # patch AdClmHub_1
      with open(adclmhub_path, 'r+b') as f:
         f.seek(seek_pos)
         letter = f.read(1)
         if letter == 't':
            print 'Patching {} ...'.format(adclmhub_name)
            f.seek(seek_pos)
            f.write(new_adlmint_last_char)
            print '{} patched!'.format(adclmhub_name)
         else:
            print '{} already patched. Skipping ...'.format(adclmhub_name)

      # rename adlmint.dll
      if os.path.isfile(new_adlmint_path):
         print 'Removing old {} ...'.format(new_adlmint_name)
         os.remove(new_adlmint_path)
      print 'Renaming {} to {} ...'.format(adlmint_name, new_adlmint_name)
      os.rename(adlmint_path, new_adlmint_path)

      print 'done patching ADLM.'

   else:
      print 'No need to patch.'

env['BUILDERS']['Patch']  = Builder(action = Action(patch_adlm,  "Patching AdLM ..."))

PATCH = env.Patch('patch', SITOA)

################################
## INSTALL TO WORKGROUP
################################

env.Install(os.path.join(env['TARGET_WORKGROUP_PATH'], bin_path), [str(SITOA[0]),
                                                                   str(SITOA_SHADERS[0])])

env.Install(os.path.join(env['TARGET_WORKGROUP_PATH'], bin_path), [glob.glob(os.path.join(ARNOLD_BINARIES, '*'))])
env.Install(os.path.join(env['TARGET_WORKGROUP_PATH'], bin_path, '..'), [glob.glob(ARNOLD_PLUGINS)])

# Copying Scripting Plugins 
# (if you modify the files directly on workgroup they will be overwritted with trunk version)
env.Install(os.path.join(env['TARGET_WORKGROUP_PATH'], plugins_path), [glob.glob(os.path.join('plugins', 'helpers', '*.js'))])

env.Install(os.path.join(env['TARGET_WORKGROUP_PATH'], plugins_path), [glob.glob(os.path.join('plugins', 'helpers', '*.py'))])

# Copy the logo pic (or whatever other bmp file found in helpers/Pictures) under Plugins\Pictures
env.Install(os.path.join(env['TARGET_WORKGROUP_PATH'], pictures_path), [glob.glob(os.path.join('plugins', 'helpers', 'Pictures', '*.bmp'))])

env.Install(os.path.join(env['TARGET_WORKGROUP_PATH'], license_path), [glob.glob(os.path.join(ARNOLD_HOME, 'license', '*'))])
env.Install(os.path.join(env['TARGET_WORKGROUP_PATH'], pit_path), [glob.glob(os.path.join(ARNOLD_HOME, 'license', 'pit', '*'))])
env.Install(os.path.join(env['TARGET_WORKGROUP_PATH'], bin_path), [glob.glob(os.path.join('shaders', 'metadata', '*.mtd'))])
env.Install(os.path.join(env['TARGET_WORKGROUP_PATH'], bin_path), [glob.glob(os.path.join('plugins', 'metadata', '*.mtd'))])

################################
## TARGETS ALIASES AND DEPENDENCIES
################################

if system.os() == 'windows':
   env.Depends(SOLUTION, SITOA_PRJ)
   env.Depends(SOLUTION, SITOA_SHADERS_PRJ)
   env.Depends(SOLUTION, INSTALL_PRJ)
   env.AlwaysBuild(INSTALL_PRJ)
   top_level_alias(env, 'solution', SOLUTION)

top_level_alias(env, 'sitoa', SITOA)
top_level_alias(env, 'shaders', SITOA_SHADERS)
top_level_alias(env, 'pack', PACKAGE)
top_level_alias(env, 'deploy', DEPLOY)
top_level_alias(env, 'install', env['TARGET_WORKGROUP_PATH'])
top_level_alias(env, 'testsuite', TESTSUITE)
top_level_alias(env, 'patch', PATCH)
env.AlwaysBuild(PACKAGE)
env.AlwaysBuild('install')

env.Depends(PACKAGE, SITOA)
env.Depends(PACKAGE, SITOA_SHADERS)
env.Depends(DEPLOY, PACKAGE)
env.Depends('install', SITOA)
env.Depends('install', SITOA_SHADERS)

Default(['sitoa', 'shaders'])

## Process top level aliases into the help message
Help('''%s

Top level targets:
    %s

Individual tests can be run using the 'test_nnnn' target.
Note that this folder must fall within the TEST_PATTERN glob.
''' % (vars.GenerateHelpText(env), get_all_aliases()))
