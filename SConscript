#
# Copyright 2010-2013 Fabric Engine Inc. All rights reserved.
#

import os, sys, platform, copy

Import('parentEnv', 'FABRIC_CAPI_DIR', 'FABRIC_SPLICE_VERSION', 'STAGE_DIR', 'FABRIC_BUILD_OS', 'FABRIC_BUILD_TYPE', 'NUKE_INCLUDE_DIR', 'NUKE_LIB_DIR','NUKE_VERSION', 'sharedCapiFlags', 'spliceFlags')

env = parentEnv.Clone()

nukeFlags = {
  'CPPPATH': [
      NUKE_INCLUDE_DIR
    ],
  'LIBPATH': [
    NUKE_LIB_DIR,
  ],
  'LIBS': [
    'DDImage'
  ]
}

if FABRIC_BUILD_OS == 'Windows':
  nukeFlags['CCFLAGS'] = ['/DNT_PLUGIN']
elif FABRIC_BUILD_OS == 'Linux':
  nukeFlags['CCFLAGS'] = ['-DLINUX']

env.MergeFlags(nukeFlags)
env.Append(CPPDEFINES = ["_SPLICE_NUKE_VERSION="+str(NUKE_VERSION[:4])])

env.MergeFlags(sharedCapiFlags)
env.MergeFlags(spliceFlags)

target = 'FabricSpliceNuke' + NUKE_VERSION

if FABRIC_BUILD_OS == 'Windows':
  target += '.dll'
elif FABRIC_BUILD_OS == 'Linux':
  target += '.so'
else:
  target += '.dylib'

nukeModule = env.SharedLibrary(target = target, source = Glob('*.cpp'))

nukeFiles = []

installedModule = env.Install(STAGE_DIR, nukeModule)
nukeFiles.append(installedModule[0])

nukeFiles.append(env.Install(STAGE_DIR, env.File('license.txt')))

# also install the FabricCore dynamic library
nukeFiles.append(env.Install(STAGE_DIR, env.Glob(os.path.join(FABRIC_CAPI_DIR, 'lib', '*.so'))))
nukeFiles.append(env.Install(STAGE_DIR, env.Glob(os.path.join(FABRIC_CAPI_DIR, 'lib', '*.dylib'))))
nukeFiles.append(env.Install(STAGE_DIR, env.Glob(os.path.join(FABRIC_CAPI_DIR, 'lib', '*.dll'))))

# install PDB files on windows
if FABRIC_BUILD_TYPE == 'Debug' and FABRIC_BUILD_OS == 'Windows':
  env['CCPDBFLAGS']  = ['${(PDB and "/Fd%s_incremental.pdb /Zi" % File(PDB)) or ""}']
  pdbSource = nukeModule[0].get_abspath().rpartition('.')[0]+".pdb"
  pdbTarget = os.path.join(STAGE_DIR.abspath, os.path.split(pdbSource)[1])
  copyPdb = env.Command( 'copy', None, 'copy "%s" "%s" /Y' % (pdbSource, pdbTarget) )
  env.Depends( copyPdb, installedModule )
  env.AlwaysBuild(copyPdb)

# todo: install the python client

# # install extensions
# nukeFiles.extend(installExtensions(os.path.join(STAGE_DIR.abspath, 'Exts'), env, installedModule))

alias = env.Alias('splicenuke', nukeFiles)
spliceData = (alias, nukeFiles)
Return('spliceData')
