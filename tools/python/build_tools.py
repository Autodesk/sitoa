# vim: filetype=python

## load our own python modules
import system

import os, string, platform, subprocess, shutil

import re

## create a top level alias so that the help system will know about it
ALIASES = ''
def top_level_alias(env, name, targets):
   global ALIASES
   ALIASES = '%s %s' % (ALIASES, name)
   env.Alias(name, targets)

def get_all_aliases():
   return ALIASES
   
## Searches for file in the system path. Returns a list of directories containing file
def find_in_path(file):
   path = os.environ['PATH']
   path = string.split(path, os.pathsep)
   return filter(os.path.exists, map(lambda dir, file=file: os.path.join(dir, file), path))

## Returns a list of all files with an extension from the 'valid_extensions' list
def find_files_recursive(path, valid_extensions):
   path = os.path.normpath(path)
   list = []
   for root, dirs, files in os.walk(path):
      if '.svn' in dirs:
         dirs.remove('.svn')
      for f in files:
         if os.path.splitext(f)[1] in valid_extensions:
            # Build the absolute path and then remove the root path, to get the relative path from root
            file = os.path.join(root, f)[len(path) + 1:]
            list += [file]
   return list

## Copy directories recursively, ignoring .svn dirs
## <dest> directory must not exist
def copy_dir_recursive(src, dest):
   for f in os.listdir(src):
      src_path  = os.path.join(src, f)
      dest_path = os.path.join(dest, f)
      if os.path.isdir(src_path):
         if f != '.svn':
            if not os.path.exists(dest_path):
               os.makedirs(dest_path)
            #shutil.copystat(src_path, dest_path)
            copy_dir_recursive(src_path, dest_path)
      else:
         shutil.copy2(src_path, dest_path)

## handy function to remove files only if they exist
def saferemove(path):
   if os.path.exists(path):
      os.remove(path)

## translates a process return code (as obtained by os.system or subprocess) into a status string
def process_return_code(retcode):
   if retcode == 0:
      status = 'OK'
   else:
      if system.os() == 'windows':
         if retcode < 0:
            status = 'CRASHED'
         else:
            status = 'FAILED'
      else:
         if retcode > 128:
            status = 'CRASHED'
         else:
            status = 'FAILED'
   return status      

# returns the softimage version
def get_softimage_version(sdk_path):
   softimage_version = 'unknown'
   rx = re.compile('#\s*define\s+XSISDK_VERSION\s+(\S+)') 
   try:
      for line in open(os.path.join(sdk_path, 'include', 'xsi_version.h')): 
         result = rx.search(line) 
         if result: 
            softimage_version = result.group(1) 
            break
   except:
      pass
   return softimage_version
   
## Obtains SItoA version by parsing 'Version.cpp'
def get_sitoa_version(path, components = 3):
   MAJOR_VERSION=''
   MINOR_VERSION=''
   FIX_VERSION=''

   f = open(path, 'r')
   while True:
      line = f.readline().lstrip(' \t')
      if line == "":
         # We have reached the end of file.
         break
      if line.startswith('#define'):
         tokens = line.split()
         if tokens[1] == 'SITOA_MAJOR_VERSION_NUM':
            MAJOR_VERSION = tokens[2]
         elif tokens[1] == 'SITOA_MINOR_VERSION_NUM':
            MINOR_VERSION = tokens[2]
         elif tokens[1] == 'SITOA_FIX_VERSION':
            FIX_VERSION = tokens[2][1:].strip('"')
   f.close()
   
   version = ''
   if (components > 0):
      version += MAJOR_VERSION
   if (components > 1):
      version += '.' + MINOR_VERSION
   if (components > 2):
      version += '.' + FIX_VERSION
   return version      

## Obtains Arnold library version by parsing 'ai_version.h'
def get_arnold_version(path, components = 4):
   ARCH_VERSION=''
   MAJOR_VERSION=''
   MINOR_VERSION=''
   FIX_VERSION=''

   f = open(path, 'r')
   while True:
      line = f.readline().lstrip(' \t')
      if line == "":
         # We have reached the end of file.
         break
      if line.startswith('#define'):
         tokens = line.split()
         if tokens[1] == 'AI_VERSION_ARCH_NUM':
            ARCH_VERSION = tokens[2]
         elif tokens[1] == 'AI_VERSION_MAJOR_NUM':
            MAJOR_VERSION = tokens[2]
         elif tokens[1] == 'AI_VERSION_MINOR_NUM':
            MINOR_VERSION = tokens[2]
         elif tokens[1] == 'AI_VERSION_FIX':
            FIX_VERSION = tokens[2].strip('"')
   f.close()
   
   if (components > 0):
      version = ARCH_VERSION
   if (components > 1):
      version += '.' + MAJOR_VERSION
   if (components > 2):
      version += '.' + MINOR_VERSION
   if (components > 3):
      version += '.' + FIX_VERSION
   return version      

## This function will give us the information we need about the latest snv revision of the root arnold directory
def get_latest_revision():
   p = subprocess.Popen('svn info .', shell=True, stdout = subprocess.PIPE)
   retcode = p.wait()
  
   revision = 'not found'
   url      = 'not found'
   
   for line in p.stdout:
      if line.startswith('URL:'):
         url = line[5:].strip()
      elif line.startswith('Last Changed Rev:'):
         revision = 'r' + line[18:].strip()

   return (revision, url)

def add_to_library_path(env, new_path):
   if system.os() == 'windows':
      var_name = 'PATH'
   elif system.os() == 'darwin':
      var_name = 'DYLD_LIBRARY_PATH'
   else:
      var_name = 'LD_LIBRARY_PATH'
      
   if env['ENV'].has_key(var_name):
      env['ENV'][var_name] = '%s%s%s' % (new_path, os.pathsep, env['ENV'][var_name])
   else:
      env['ENV'][var_name] = new_path

def set_library_path(env):
   if system.os() == 'windows':
      var_name = 'PATH'
   elif system.os() == 'darwin':
      var_name = 'DYLD_LIBRARY_PATH'
   else:
      var_name = 'LD_LIBRARY_PATH'

   env['PREVIOUS_LIBRARY_PATH'] = ''
   if os.environ.has_key(var_name):
      env['PREVIOUS_LIBRARY_PATH'] = os.environ[var_name]
   os.environ[var_name] = env['ENV'][var_name]

def reset_library_path(env):
   if env.has_key('PREVIOUS_LIBRARY_PATH'):
      if system.os() == 'windows':
         var_name = 'PATH'
      elif system.os() == 'darwin':
         var_name = 'DYLD_LIBRARY_PATH'
      else:
         var_name = 'LD_LIBRARY_PATH'
      os.environ[var_name] = env['PREVIOUS_LIBRARY_PATH']
   
def add_to_program_path(env, new_path):
   if env['ENV'].has_key('PATH'):
      env['ENV']['PATH'] = '%s%s%s' % (new_path, os.pathsep, env['ENV']['PATH'])
   else:
      env['ENV']['PATH'] = new_path

def set_program_path(env):
   env['PREVIOUS_PROGRAM_PATH'] = ''
   if os.environ.has_key('PATH'):
      env['PREVIOUS_PROGRAM_PATH'] = os.environ['PATH']
   os.environ['PATH'] = env['ENV']['PATH']

def reset_program_path(env):
   if env.has_key('PREVIOUS_PROGRAM_PATH'):
      os.environ['PATH'] = env['PREVIOUS_PROGRAM_PATH']
   
def get_default_path(var, default):
   if var in os.environ:
      return os.environ[var]
   else:  
      return default 

def get_escaped_path(path):
   if system.os() == 'windows':
      return path.replace("\\", "\\\\")
   else:
      return path

## Hacky replacement for the string partition method which is only available from Python 2.5
def strpartition(string, sep):
   index = string.find(sep)
   if index == -1:
      return (string, '', '')
   return (string[:index], sep, string[index + 1:])

   
def get_library_extension():
   if system.os() == 'windows':
      return ".dll"
   elif system.os() == 'linux':
      return ".so"
   elif system.os() == 'darwin':
      return ".dylib"
   else:
      return ""
      
def get_executable_extension():
   if system.os() == 'windows':
      return ".exe"
   else:
      return ""         