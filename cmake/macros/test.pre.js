//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
Module = Module || {};
Module.preRun = Module.preRun || [];

// Test runner will handle copying test assets to a temporary directory.
// In order to run the test in node, we want to map that temporary directory
// into the virtual file system so that all files are available and any files
// created during the test will be persisted for use in comparison diffs.
// Note: relevant plugInfo.json files are embedded in the test binary itself.
Module.preRun.push(function() {
	FS.mkdir('/test');
	FS.mount(FS.filesystems.NODEFS, { root: process.cwd() }, '/test');
	FS.chdir('/test')
});
