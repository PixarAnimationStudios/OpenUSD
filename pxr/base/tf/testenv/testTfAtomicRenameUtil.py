#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
import argparse
import glob
import math
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import time
import unittest

from typing import Callable, List, Optional

TEST_FILE_BASE = "atomicRenameTestFile"
TEST_FILE_FINAL = TEST_FILE_BASE + ".final"
TEST_FILE_GLOB = TEST_FILE_BASE + ".*"
OLD_CONTENT = "The old stuff"
EXPECTED_NEW_CONTENT = "New Content"
SENTRY_FILENAME = "wait_for_me.txt"

# We do a high number of retries, with a longish wait, to try to ensure success
NUM_RETRIES = 100
RETRY_WAIT_SEC = 0.03

# this timeout should be long enough to ensure that we either run out of retries, or finish successfully
TIMEOUT = 5.0

testAtomicRenameUtil_exe_path = ""


if platform.system() == "Windows":

    def launch_testAtomicRenameUtil(
        do_retry: bool,
        retry_wait_seconds: float,
        folder: str,
        wait_for_file: str = "",
    ) -> subprocess.Popen:
        env = dict(os.environ)
        env["TF_FILE_LOCK_NUM_RETRIES"] = str(NUM_RETRIES if do_retry else 0)
        env["TF_FILE_LOCK_RETRY_WAIT_MS"] = str(int(retry_wait_seconds * 1000))
        args = [testAtomicRenameUtil_exe_path, os.path.join(folder, TEST_FILE_BASE)]
        if wait_for_file:
            args.append(wait_for_file)
        return subprocess.Popen(args, env=env)

    def get_temp_files(folder: str) -> "List[str]":
        all_matches = glob.glob(os.path.join(folder, TEST_FILE_GLOB))
        return [x for x in all_matches if x != os.path.join(folder, TEST_FILE_FINAL)]

    def folder_perms_deny(path: str):
        subprocess.check_call(["icacls", path, "/deny", "EVERYONE:(DC,WD)"])

    def folder_perms_reset(path: str):
        subprocess.check_call(["icacls", path, "/reset", "/t", "/c"])

    class TestAtomicRenameUtil(unittest.TestCase):
        def __init__(self, *args, **kwargs):
            self.retry_wait_seconds = RETRY_WAIT_SEC
            # top level directory that we will clean up when done
            self.tempdir = ""
            # directory that will contain the test output file - should either
            # be self.tempdir or a subdirectory
            self.testingdir = ""
            self.timer_start = -math.inf
            super().__init__(*args, **kwargs)

        def assert_new_content(self):
            with open(os.path.join(self.testingdir, TEST_FILE_FINAL), "r", encoding="utf8") as reader:
                content = reader.read().strip()
            self.assertEqual(content, EXPECTED_NEW_CONTENT)

        def _assert_testAtomicRenameUtil(
            self,
            do_retry: bool,
            expect_success: bool,
            wait_for_file: str = "",
            post_launch_callback: Optional[Callable[[subprocess.Popen], None]] = None,
        ) -> bool:
            """
            Run our testAtomicRenameUtil test executable, to verify correct operation of
            TfSafeOutputFile::Replace / TfAtomicRenameUtil

            Can optionally provide a wait_for_file to signal testAtomicRenameUtil to
            pause after creating temp files, but before doing the final move (and
            invoking TfAtomicRenameUtil).

            Can also provide a post_launch_callback, which will be executed after the
            testAtomicRenameUtil process is launched.  This can be used in conjunction with
            wait_for_file to do some actions at a point when we know the process is running,
            but before it has invoked TfAtomicRenameUtil.
            """
            expected_str = "succeed" if expect_success else "fail"
            # Print to sys.stderr, since that's what testAtomicRenameUtil will print to if it fails
            print("#" * 80, file=sys.stderr, flush=True)
            print(f"Running {testAtomicRenameUtil_exe_path} - expected to {expected_str}", file=sys.stderr, flush=True)
            proc = launch_testAtomicRenameUtil(
                do_retry,
                self.retry_wait_seconds,
                self.testingdir,
                wait_for_file=wait_for_file)
            try:
                if post_launch_callback is not None:
                    post_launch_callback(proc)
                proc.wait(TIMEOUT)
                temp_files = get_temp_files(self.testingdir)
                # Clean up any leftover temp files
                for temp_file in temp_files:
                    os.unlink(temp_file)
                print(f"  ...done running {testAtomicRenameUtil_exe_path}", file=sys.stderr, flush=True)
                if expect_success:
                    self.assertEqual(proc.returncode, 0)
                    self.assertFalse(temp_files)
                    self.assert_new_content()
                else:
                    self.assertNotEqual(proc.returncode, 0)
            finally:
                proc.terminate()

        def assertSuccess_testAtomicRenameUtil(
            self,
            do_retry: bool,
            wait_for_file: str = "",
            post_launch_callback: Optional[Callable[[subprocess.Popen], None]] = None,
        ):
            self._assert_testAtomicRenameUtil(
                do_retry, True, wait_for_file=wait_for_file, post_launch_callback=post_launch_callback
            )

        def assertFailure_testAtomicRenameUtil(
            self,
            do_retry: bool,
            wait_for_file: str = "",
            post_launch_callback: Optional[Callable[[subprocess.Popen], None]] = None,
        ):
            self._assert_testAtomicRenameUtil(
                do_retry, False, wait_for_file=wait_for_file, post_launch_callback=post_launch_callback
            )

        def wait_for_temp_files(self, proc: subprocess.Popen):
            start = time.perf_counter()
            while not get_temp_files(self.testingdir):
                time.sleep(RETRY_WAIT_SEC)
                elapsed = time.perf_counter() - start
                if elapsed >= TIMEOUT:
                    proc.terminate()
                    self.fail("Timed out waiting for temp file to be created")

        def setUp(self):
            self.assertTrue(os.path.isfile(testAtomicRenameUtil_exe_path))
            self.tempdir = tempfile.mkdtemp(dir=os.getcwd())
            self.testingdir = self.tempdir

        def tearDown(self):
            if self.tempdir:
                shutil.rmtree(self.tempdir)

        def test_fileLockedRetry(self):
            """
            Test retrying if another process has a lock on the file we wish to write over
            """
            # first, test that if no handles are open to test_file_final, it works, with or without retries
            self.assertSuccess_testAtomicRenameUtil(do_retry=False)
            self.assertSuccess_testAtomicRenameUtil(do_retry=True)

            sentry_file = os.path.join(self.testingdir, SENTRY_FILENAME)
            final_file = os.path.join(self.testingdir, TEST_FILE_FINAL)
            with open(final_file, "w", encoding="utf8") as writer:
                # Grab a handle, and write old content to file
                writer.write(OLD_CONTENT)

                # Now try running testAtomicRenameUtil without retries - it should fail
                self.assertFailure_testAtomicRenameUtil(do_retry=False)

                self.assertFalse(get_temp_files(self.testingdir))

                # Now we get our "real" test that our rety-on-lock works.
                # Rather than just trying to grab a lock a bunch of times, at
                # the same time we try to write a bunch of times, and hope that
                # there's some collisions, we try to ENSURE that there is a
                # collision - but this takes some coordination between this
                # process (the TEST PROCESS, which is monitoring and creating
                # locks), and the WRITING PROCESS, which is actually running
                # TfSafeOutputFile::Replace / TfAtomicRenameUtil.

                # The intended flow is:
                # - in this TEST PROCESS:
                #   - launch the WRITING PROCESS
                # - in the WRITING PROCESS:
                #   - start TfSafeOutputFile::Replace, which will
                #     create the temporary files (but not commit / do final move)
                #   - then pause, until a sentry file is created
                # - in this TEST PROCESS:
                #   - we wait until the temp file is created
                #   - then we grab a lock on the final file
                #   - then we create the sentry file
                #   - then we sleep / hold onto the lock for shortish time
                #     (ie, an amount of time that should be < the total amount
                #     of retry time in the WRITING PROCESS)
                # - in the WRITING PROCESS:
                #   - we see that the sentry file has been created
                #   - we try to commit / do the final move
                #   - this should initially FAIL, because the TEST PROCESS
                #     has a lock on the final file
                #   - we enter our retry loop
                # - in this TEST PROCESS:
                #   - we finish our short sleep, and release the lock on the
                #     final file
                # - in the WRITING PROCESS:
                #   - we should finally succeed, because the lock has been released

                def wait_for_temp_then_create_then_release(proc: subprocess.Popen):
                    # wait until a temp file shows up, so we know that the proc we launched is running
                    self.wait_for_temp_files(proc)
                    # create the wait_for_file
                    with open(sentry_file, "w", encoding="utf8") as _writer:
                        pass
                    # Do one more sleep, just to ensure we hang onto the lock for a bit
                    time.sleep(RETRY_WAIT_SEC)
                    # then release the lock
                    writer.close()

                # Now try launching testAtomicRenameUtil with retries, and pass a callback which waits for
                # temp files, then creates the wait_for_file, then releases the lock
                self.assertSuccess_testAtomicRenameUtil(
                    do_retry=True,
                    wait_for_file=sentry_file,
                    post_launch_callback=wait_for_temp_then_create_then_release
                )

        def test_noFilePermsNoRetry(self):
            """Test that if we don't have permissions to move, we fail immediately without retry/waiting"""
            # set a long retry wait time, so we can be sure whether or not it retried
            self.retry_wait_seconds = 1.0

            # check that we fail quickly, both if retries are on or off
            for do_retries in (False, True):
                sentry_path = os.path.join(self.tempdir, f"sentry_file_do_retry_{do_retries}")

                # We need to make a directory that tests will be run in,
                # that we can lock; we need to leave the permissions
                # on tempdir writable, as that is where we will be writing
                # our sentry file, which needs to happen AFTER we lock the
                # directory
                self.testingdir = os.path.join(self.tempdir, f"do_retries_{do_retries}")
                os.mkdir(self.testingdir)

                # we want to test that what happens if there is a permissions error when we try to "commit" the
                # TfSafeOutputFile
                # we do this by locking down the directory perms on tempdir

                # However, if we do that before even launching testAtomicRenameUtil, then it won't even be able to
                # create the temp files, and will never even get to the point where it tries to commit

                # so we need to make a callback that locks down the directory perms AFTER the proc is launched, and we
                # can confirm temp files were made

                def wait_for_temp_then_lock_then_create(proc: subprocess.Popen):
                    # wait until a temp file shows up, so we know that the proc we launched is running
                    self.wait_for_temp_files(proc)

                    # now lock the directory
                    folder_perms_deny(self.testingdir)

                    # then create the wait_for_file
                    with open(sentry_path, "w", encoding="utf8") as _writer:
                        pass
                    # then start our timer
                    self.timer_start = time.perf_counter()

                try:
                    self.assertFailure_testAtomicRenameUtil(
                        do_retries,
                        wait_for_file=sentry_path,
                        post_launch_callback=wait_for_temp_then_lock_then_create,
                    )
                    timer_end = time.perf_counter()
                    # Check that it didn't retry, and failed quickly
                    self.assertLess(timer_end - self.timer_start, self.retry_wait_seconds)
                finally:
                    folder_perms_reset(self.testingdir)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("testAtomicRenameUtil_exe_path")
    known_args, unittest_args = parser.parse_known_args()

    testAtomicRenameUtil_exe_path = known_args.testAtomicRenameUtil_exe_path

    unittest_args.insert(0, sys.argv[0])
    unittest.main(argv=unittest_args)
