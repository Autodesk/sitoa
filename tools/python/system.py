# vim: filetype=python

import platform, string

from os import environ   # To avoid name clashes with os() function

OS          = ''
HOST_ARCH   = ''
TARGET_ARCH = ''
VALID_OSES  = ['linux', 'darwin', 'windows']

## Returns the name of the OS ('windows', 'linux', darwin')
def os():
   return OS

## Returns host system architecture ('x86', 'x86_64', 'sparc_64')
def host_arch():
   return HOST_ARCH
   
## Returns target system architecture ('x86', 'x86_64', 'sparc_64')
def target_arch():
   return TARGET_ARCH

## Returns a list of valid operating systems
def get_valid_oses():
   return VALID_OSES
   
## Returns a list of valid target architectures
def get_valid_target_archs():
   return VALID_ARCHS
   
## Returns a simplified label for the system architecture ('linux_x86', 'linux_x86_64', 'darwin32', 'win32')
def get_arch_label(os, arch):
   if OS == 'windows':
      if arch == 'x86':
         return 'win32'
      elif arch == 'x86_64':
         return 'win64'
   elif OS == 'darwin':
      if arch == 'x86':
         return 'darwin32'
   elif OS == 'linux':
      if arch == 'x86':
         return 'linux_x86'
      elif arch == 'x86_64':
         return 'linux_x86_64'
      elif arch == 'sparc64':
         return 'linux_sparc_64'

   return 'Unknown'

## Selects a new target architecture
def set_target_arch(arch):
   global TARGET_ARCH, TARGET_ARCH_LABEL
   if not arch in VALID_ARCHS:
      print "ERROR: Target architecture is not valid"
   else:
      TARGET_ARCH = arch
      TARGET_ARCH_LABEL = get_arch_label(OS, arch)

# Obtain information about the system only once, when loaded
OS = platform.system().lower()

if OS == 'windows':
   VALID_ARCHS = ('x86', 'x86_64')
   if (environ.has_key('PROCESSOR_ARCHITECTURE') and environ['PROCESSOR_ARCHITECTURE'] == 'AMD64') or (environ.has_key('PROCESSOR_ARCHITEW6432') and environ['PROCESSOR_ARCHITEW6432'] == 'AMD64'):
      HOST_ARCH = 'x86_64'
   else:
      HOST_ARCH = 'x86'
elif OS == 'darwin':
   VALID_ARCHS = ('x86', 'x86_64')

   if platform.architecture()[0] == '64bit':
      HOST_ARCH = 'x86_64'
   else:
      HOST_ARCH = 'x86'
elif OS == 'linux':
   VALID_ARCHS = ('x86', 'x86_64', 'sparc_64')
   
   if platform.machine() == 'sparc64':
      HOST_ARCH = 'sparc_64'
   elif platform.machine() == 'x86_64':
      HOST_ARCH = 'x86_64'
   else:
      HOST_ARCH = 'x86'

TARGET_ARCH = HOST_ARCH
