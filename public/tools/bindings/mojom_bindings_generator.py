#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The frontend for the Mojo bindings system."""


import argparse
import imp
import os
import pprint
import sys

# Disable lint check for finding modules:
# pylint: disable=F0401

# Manually check for the command-line flag. (This isn't quite right, since it
# ignores, e.g., "--", but it's close enough.)
try:
  index = sys.argv.index("--bundled_pylibs_root") + 1
  bundled_pylibs_root = sys.argv[index]
  sys.path.insert(0, bundled_pylibs_root)
except ValueError:
  pass


sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "pylib"))

from mojom.error import Error
from mojom.generate.data import OrderedModuleFromData
from mojom.parse.parser import Parse
from mojom.parse.translate import Translate


def LoadGenerators(generators_string):
  if not generators_string:
    return []  # No generators.

  script_dir = os.path.dirname(os.path.abspath(__file__))
  generators = []
  for generator_name in [s.strip() for s in generators_string.split(",")]:
    # "Built-in" generators:
    if generator_name.lower() == "c++":
      generator_name = os.path.join(script_dir, "generators",
                                    "mojom_cpp_generator.py")
    elif generator_name.lower() == "javascript":
      generator_name = os.path.join(script_dir, "generators",
                                    "mojom_js_generator.py")
    elif generator_name.lower() == "java":
      generator_name = os.path.join(script_dir, "generators",
                                    "mojom_java_generator.py")
    elif generator_name.lower() == "python":
      generator_name = os.path.join(script_dir, "generators",
                                    "mojom_python_generator.py")
    # Specified generator python module:
    elif generator_name.endswith(".py"):
      pass
    else:
      print "Unknown generator name %s" % generator_name
      sys.exit(1)
    generator_module = imp.load_source(os.path.basename(generator_name)[:-3],
                                       generator_name)
    generators.append(generator_module)
  return generators


def MakeImportStackMessage(imported_filename_stack):
  """Make a (human-readable) message listing a chain of imports. (Returned
  string begins with a newline (if nonempty) and does not end with one.)"""
  return ''.join(
      reversed(["\n  %s was imported by %s" % (a, b) for (a, b) in \
                    zip(imported_filename_stack[1:], imported_filename_stack)]))


def FindImportFile(dir_name, file_name, search_dirs):
  for search_dir in [dir_name] + search_dirs:
    path = os.path.join(search_dir, file_name)
    if os.path.isfile(path):
      return path
  return os.path.join(dir_name, file_name)

class MojomProcessor(object):
  def __init__(self, should_generate):
    self._should_generate = should_generate
    self._processed_files = {}

  def ProcessFile(self, args, remaining_args, generator_modules, filename,
                  _imported_filename_stack=None):
    # Memoized results.
    if filename in self._processed_files:
      return self._processed_files[filename]

    if _imported_filename_stack is None:
      _imported_filename_stack = []

    # Ensure we only visit each file once.
    if filename in _imported_filename_stack:
      print "%s: Error: Circular dependency" % filename + \
          MakeImportStackMessage(_imported_filename_stack + [filename])
      sys.exit(1)

    try:
      with open(filename) as f:
        source = f.read()
    except IOError as e:
      print "%s: Error: %s" % (e.filename, e.strerror) + \
          MakeImportStackMessage(_imported_filename_stack + [filename])
      sys.exit(1)

    try:
      tree = Parse(source, filename)
    except Error as e:
      full_stack = _imported_filename_stack + [filename]
      print str(e) + MakeImportStackMessage(full_stack)
      sys.exit(1)

    dirname, name = os.path.split(filename)
    mojom = Translate(tree, name)
    if args.debug_print_intermediate:
      pprint.PrettyPrinter().pprint(mojom)

    # Process all our imports first and collect the module object for each.
    # We use these to generate proper type info.
    for import_data in mojom['imports']:
      import_filename = FindImportFile(dirname,
                                       import_data['filename'],
                                       args.import_directories)
      import_data['module'] = self.ProcessFile(
          args, remaining_args, generator_modules, import_filename,
          _imported_filename_stack=_imported_filename_stack + [filename])

    module = OrderedModuleFromData(mojom)

    # Set the path as relative to the source root.
    module.path = os.path.relpath(os.path.abspath(filename),
                                  os.path.abspath(args.depth))

    # Normalize to unix-style path here to keep the generators simpler.
    module.path = module.path.replace('\\', '/')

    if self._should_generate(filename):
      for generator_module in generator_modules:
        generator = generator_module.Generator(module, args.output_dir)
        filtered_args = []
        if hasattr(generator_module, 'GENERATOR_PREFIX'):
          prefix = '--' + generator_module.GENERATOR_PREFIX + '_'
          filtered_args = [arg for arg in remaining_args
                           if arg.startswith(prefix)]
        generator.GenerateFiles(filtered_args)

    # Save result.
    self._processed_files[filename] = module
    return module

def main():
  parser = argparse.ArgumentParser(
      description="Generate bindings from mojom files.")
  parser.add_argument("filename", nargs="+",
                      help="mojom input file")
  parser.add_argument("-d", "--depth", dest="depth", default=".",
                      help="depth from source root")
  parser.add_argument("-o", "--output_dir", dest="output_dir", default=".",
                      help="output directory for generated files")
  parser.add_argument("-g", "--generators", dest="generators_string",
                      metavar="GENERATORS",
                      default="c++,javascript,java,python",
                      help="comma-separated list of generators")
  parser.add_argument("--debug_print_intermediate", action="store_true",
                      help="print the intermediate representation")
  parser.add_argument("-I", dest="import_directories", action="append",
                      metavar="directory", default=[],
                      help="add a directory to be searched for import files")
  parser.add_argument("--bundled_pylibs_root", dest=bundled_pylibs_root,
                      default=None,
                      help="Directory storing bundled Python modules")
  (args, remaining_args) = parser.parse_known_args()

  generator_modules = LoadGenerators(args.generators_string)

  if not os.path.exists(args.output_dir):
    os.makedirs(args.output_dir)

  processor = MojomProcessor(lambda filename: filename in args.filename)
  for filename in args.filename:
    processor.ProcessFile(args, remaining_args, generator_modules, filename)

  return 0


if __name__ == "__main__":
  sys.exit(main())
