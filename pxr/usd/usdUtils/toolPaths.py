#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
import os
import platform
import subprocess
import sys
import shutil

# Characters that cmd.exe treats specially. A .cmd wrapper re-expands its
# arguments through the cmd.exe parser, so these characters can alter the
# command being run even when the arguments were passed as a list rather than
# as a shell string. See _ResolveUsdCommand for how this is avoided.
_CMD_METACHARACTERS = frozenset('&|<>^%()"\r\n')

def FindUsdBinary(name):
    """Returns the full path to the named executable if it can be found, or
    None if the executable cannot be located. This first searches in PATH, and
    if the executable is not found, it then searches in the parent directory
    of the current process, as identified by sys.argv[0].

    On Windows, this function searches for both name.EXE and name.CMD to
    ensure that CMD-wrapped executables are located if they exist.
    """

    # First search PATH
    binpath = shutil.which(name)
    if binpath:
        return binpath

    # Then look relative to the current executable
    binpath = shutil.which(name,
        path=os.path.abspath(os.path.dirname(sys.argv[0])))
    if binpath:
        return binpath

    if platform.system() == 'Windows':
        # shutil.which under Windows only returns *.EXE files so we need to
        # traverse the tool path.
        path = os.environ.get('PATH', '').split(os.pathsep)
        for base in [os.path.join(p, name) for p in path]:
            # We need to test for name.cmd first because on Windows, the USD
            # executables are wrapped due to lack of UNIX style shebang support.
            for binpath in [base + ext for ext in ('.cmd', '')]:
                if os.access(binpath, os.X_OK):
                    return binpath

    return None

def _GetScriptInterpreter(scriptPath):
    """For Windows only, shebang emulation: returns the interpreter named by
    scriptPath's '#!' line, or None if the file has no usable shebang. The USD
    build substitutes an absolute path to the Python that the tools were
    configured against, so preferring it over sys.executable keeps a spawned
    tool running against its own interpreter.
    """
    try:
        with open(scriptPath, 'r') as f:
            firstLine = f.readline(1024)
    except (IOError, OSError, UnicodeDecodeError):
        return None

    if not firstLine.startswith('#!'):
        return None

    interpreter = firstLine[2:].strip()
    if not interpreter:
        return None

    return interpreter if os.path.isfile(interpreter) else None

def _ResolveUsdCommand(name):
    """Returns the argv prefix used to run the named USD tool, or None if the
    tool cannot be located.

    On Windows, Python tools are installed alongside a generated '<name>.cmd'
    wrapper, because Windows will not execute a script via its '#!' line. That
    wrapper forwards its arguments with '%*', which makes cmd.exe re-parse them
    and reintroduces command injection even for arguments passed as a list. To
    avoid the wrapper, this prefers the script itself and invokes its
    interpreter directly.
    """
    binpath = FindUsdBinary(name)
    if not binpath:
        return None

    if (platform.system() != 'Windows' or
        os.path.splitext(binpath)[1].lower() != '.cmd'):
        return [binpath]

    # The generated .cmd wrapper runs the identically named extensionless script
    # beside it.
    scriptPath = os.path.splitext(binpath)[0]
    if os.path.isfile(scriptPath):
        interpreter = _GetScriptInterpreter(scriptPath) or sys.executable
        if interpreter:
            return [interpreter, scriptPath]

    # Falling back to the wrapper means arguments pass through cmd.exe, so the
    # caller must reject arguments that cmd.exe would interpret.
    sys.stderr.write(
        "Warning: Found only the '%s' wrapper for '%s' and not the script it "
        "runs. Arguments will be interpreted by cmd.exe; arguments containing "
        "cmd.exe special characters will be refused.\n" % (binpath, name))
    return [binpath]

def RunUsdBinary(name, options=(), paths=(), wait=False, **kwargs):
    """Runs the named USD tool, building its argument list as

        <tool> <options> -- <paths>

    and returns the subprocess.Popen object, or the tool's exit status if wait
    is True. Raises RuntimeError if the tool cannot be found or cannot be run
    safely.

    Paths are passed after a '--' separator so that a path beginning with '-'
    is treated as an operand rather than as an option. Because no shell is
    involved, characters such as ';' and '&' are likewise never interpreted.
    This matters because paths may come from the content of an opened layer.
    Options are passed ahead of the separator and are expected to originate
    with the caller, not with layer content.

    Remaining keyword arguments are forwarded to subprocess.Popen.
    """
    command = _ResolveUsdCommand(name)
    if not command:
        raise RuntimeError(
            "Could not find '%s'. Expected it to be in PATH." % name)

    args = command + [str(o) for o in options]
    paths = [str(p) for p in paths]
    if paths:
        args += ['--'] + paths

    # If arguments will reach cmd.exe by way of a .cmd wrapper, refuse any that
    # it would interpret rather than attempting to quote them; cmd.exe quoting
    # rules do not permit this to be done reliably.
    if os.path.splitext(command[0])[1].lower() == '.cmd':
        for arg in args[1:]:
            if _CMD_METACHARACTERS.intersection(arg):
                raise RuntimeError(
                    "Refusing to run '%s' via its cmd.exe wrapper with the "
                    "argument '%s', which contains characters that cmd.exe "
                    "would interpret." % (name, arg))

    if wait:
        return subprocess.call(args, **kwargs)

    kwargs.setdefault('close_fds', True)
    return subprocess.Popen(args, **kwargs)
