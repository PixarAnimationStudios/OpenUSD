#
# Copyright 2024 Pixar
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
import sys

pythonpath = sys.argv[1]
scriptfile = sys.argv[2]
sys.argv = sys.argv[2:]
sys.path.append(pythonpath)
exec(compile(open(scriptfile).read(), scriptfile, 'exec'))
