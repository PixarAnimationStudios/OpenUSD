Filing Issues for USD
=====================

We are very excited to gather community feedback for USD. If you have found an
issue, we ask that you follow these guidelines when filing.

- List all relevant configuration information, including operating system,
software versions, hardware configuration, driver versions etc.

- Provide a "Minimal Repro", meaning the smallest set of steps necessary to 
reproduce the bug.

Below is an example bug description:

    *Title*: Usd Stage Fails to Save.

    *Content*:
    System Information:
    ```
    Operating System: Ubuntu 12.04 64Bit
    Hardware Configuration: Intel Core i5, Nvidia GTX 980(Driver 343.22)
    Intel TBB Version: 4.4 Update 5
    G++ Version: 6.1.0
    Python Version: 2.7.5
    Boost Version: 1.6.1
    OpenSubdiv Version: 3.0.5 
    GLEW Version: 1.10.0 
    OpenImageIO Version: 1.5.11
    Ptex Version: 2.0.30
    Qt Version: 4.8.1 
    Pyside Version: 1.2.2
    ```

    
    Repro case:
    1. Launch Python
    2. Run the following snippet

    ```python

        >>> from pxr import Usd
        >>> stage = Usd.Stage.CreateNew('hello.usda')
        >>> stage.GetRootLayer().Save()
    ```

    3. Note that 'hello.usda' is not created on disk, my expectation is that
    it would be

These guidelines will help us get to issues more quickly, we appreciate your
help! 
