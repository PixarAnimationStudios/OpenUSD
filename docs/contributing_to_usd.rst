===================
Contributing to USD
===================

.. include:: rolesAndUtils.rst

**We're excited to collaborate with the community and look forward to the many
improvements you can make to USD!**

Contributor License Agreement
*****************************

Before contributing code to USD, we ask that you sign a Contributor License
Agreement (CLA). At the root of the `repository
<https://github.com/PixarAnimationStudios/OpenUSD>`_ you can find the two possible
CLAs:

    #. `USD_CLA_Corporate.pdf
       <https://github.com/PixarAnimationStudios/OpenUSD/blob/release/USD_CLA_Corporate.pdf>`_
       : please sign this one for corporate use

    #. `USD_CLA_Individual.pdf
       <https://github.com/PixarAnimationStudios/OpenUSD/blob/release/USD_CLA_Individual.pdf>`_
       : please sign this one if you're an individual contributor

Once your CLA is signed, send it to `usd-cla@pixar.com
<mailto:usd-cla@pixar.com>`_ (please make sure to include your github username)
and wait for confirmation that we've received it. After that, you can submit
pull requests.

Coding Conventions
******************

Please follow the coding convention and style in each file and in each library
when adding new files.

Pull Request Guidelines
***********************

    * All development on USD should happen against the "**dev**" branch of the
      repository. Please make sure the base branch of your pull request is set
      to the "**dev**" branch when filing your pull request.

    * Please make sure all tests are passing with your change prior to
      submitting a pull request. Keep in mind the current github CI pipeline 
      does not run any tests, however tests will be run when reviewing your
      submitted change for consideration.

    * Please make sure that your pull requests are clean. Use the rebase and
      squash git facilities as needed to ensure that the pull request is as
      clean as possible.

    * Please make pull requests that are small and atomic. In general, it is
      easier for us to merge pull requests that serve a single
      purpose than those that combine several functional pieces.

    * Please search through existing open github issues and associate your PR
      with issues that your change addresses, as described in :ref:`github_issues`.
      If there are no issues related to your change, you do not need to create 
      a new issue.

Git Workflow
************

Here is the workflow we recommend for contributing changes to USD:

    #. Use the github website to fork your own private repository.

        .. | space |

    #. Clone your fork to your local machine, like this:

       .. code-block:: sh

          git clone https://github.com/you/USD.git

       

    #. Add Pixar's USD repo as upstream to make it easier to update your remote
       and local repos with the latest changes:

       .. code-block:: sh

          cd USD
          git remote add upstream https://github.com/PixarAnimationStudios/OpenUSD.git

       

    #. Now fetch the latest changes from Pixar's USD repo like this:

       .. code-block:: sh

          git fetch upstream

       

    #. We recommend you create a new branch for each feature or fix that you'd
       like to make and give it a descriptive name so that you can remember it
       later. You can checkout a new branch and create it simultaneously like
       this:

       .. code-block:: sh

          git checkout -b dev_mybugfix upstream/dev

       

    #. Now you can work in your branch locally.

        .. | space |

    #. Once you are happy with your change, you can verify that the change
       didn't cause tests failures by running tests from your build directory:

       .. code-block:: sh

          ctest -C Release

       

    #. If all the tests pass and you'd like to send your change in for
       consideration, push it to your remote repo:

       .. code-block:: sh

          git push origin dev_mybugfix

       

    #. Now your remote branch will have your dev_mybugfix branch, which you can
       now pull request (to USD's dev branch) using the github UI.

When your pull request is merged, it will be available in the next dev and full
release. For USD release schedules, see :ref:`release_schedule`

.. _github_issues:

Github Issues
#############

Use github issues to report problems or suggestions that need discussion, or 
that you might not be able to address yourself.

You do not need to log an issue for contributing changes, or if your change
fixes an unreported issue. However, if your change requires multiple pull 
requests, a single issue can be created and referenced from those PRs to 
organize them.

Before sending your change in for consideration, search through the list of
`open issues on github <https://github.com/PixarAnimationStudios/OpenUSD/issues>`_ 
and check if your change addresses any issue. If so, associate your pull
request with that issue.
