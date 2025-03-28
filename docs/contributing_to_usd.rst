===================
Contributing to USD
===================

.. include:: rolesAndUtils.rst

**We're excited to collaborate with the community and look forward to the many
improvements you can make to USD!**

.. _contributor_license_agreement:

*****************************
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
<mailto:usd-cla@pixar.com>`__. **Please make sure to include your GitHub username**
so we can grant your GitHub account appropriate permissions to the OpenUSD repo.

You can start to make code contributions once you’ve received confirmation that 
we've received your CLA. If you are planning on making a major change (for 
example, adding a new feature, or making a code change that modifies dozens of 
lines of code across several different files), see 
:ref:`planning_major_changes` for recommended next steps. Otherwise, for 
smaller changes, such as bugfixes, you can submit GitHub pull requests for
consideration, using the 
:ref:`pull request guidelines<pull_request_guidelines>`.

.. _coding_conventions:

******************
Coding Conventions
******************

Please review the coding conventions described in the  
`Coding Guidelines <api/_page__coding__guidelines.html>`__ and 
`Testing Guidelines <api/_page__testing__guidelines.html>`__ and follow the 
coding conventions and styles in each file and library when making changes. 

.. _pull_request_guidelines:

***********************
Pull Request Guidelines
***********************

    * All development should happen against the "**dev**" branch of the
      repository. Please make sure the base branch of your pull request is set
      to the "**dev**" branch when filing your pull request.

    * Please make pull requests that are small and atomic. In general, it is
      easier for us to merge pull requests that serve a single
      purpose than those that combine several functional pieces.

    * Please make sure that your pull requests are clean. Use the rebase and
      squash git facilities as needed to ensure that the pull request is as
      clean as possible.

    * Please make sure all tests are passing with your change prior to
      submitting a pull request. Keep in mind the current GitHub CI pipeline 
      does not run any tests, however tests will be run when reviewing your
      submitted change for consideration.

    * Please search through 
      `existing open GitHub issues <https://github.com/PixarAnimationStudios/OpenUSD/issues>`__
      and associate your PR with issues that your change addresses. If there are 
      no issues related to your change, you do not need to create a new issue. 
      However, if your change requires multiple pull requests, it can be helpful 
      to create a single issue to link together and organize related PRs.

.. _git_workflow:

************
Git Workflow
************

Here is the workflow we recommend for contributing changes to OpenUSD:

    #. Use the GitHub website to fork your own private repository.

        .. | space |

    #. Clone your fork to your local machine, like this:

       .. code-block:: sh

          git clone https://github.com/you/USD.git

       

    #. Add Pixar's OpenUSD repo as upstream to make it easier to update your remote
       and local repos with the latest changes:

       .. code-block:: sh

          cd USD
          git remote add upstream https://github.com/PixarAnimationStudios/OpenUSD.git

       

    #. Now fetch the latest changes from Pixar's OpenUSD repo like this:

       .. code-block:: sh

          git fetch upstream

       

    #. We recommend you create a new branch for each feature or fix that you'd
       like to make and give it a descriptive name so that you can remember it
       later. You can checkout a new branch and create it simultaneously like
       this:

       .. code-block:: sh

          git checkout -b dev_mybugfix upstream/dev

       

    #. Now you can work in your branch locally. Please review the 
       `Coding Guidelines <api/_page__coding__guidelines.html>`__ and 
       `Testing Guidelines <api/_page__testing__guidelines.html>`__ 
       when making code changes.

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
       now pull request (to OpenUSD's dev branch) using the GitHub UI.

When your pull request is merged, it will be available in the next dev and full
release. For OpenUSD release schedules, see :ref:`release_schedule`.

.. _github_issues:

GitHub Issues
=============

Use GitHub issues to report problems or suggestions that need discussion, or 
that you might not be able to address yourself. Please check that your issue 
does not already exist in the list of 
`open issues on GitHub <https://github.com/PixarAnimationStudios/OpenUSD/issues>`__ 
before submitting to avoid duplicates.  

When new issues are filed in GitHub, Pixar makes a copy in our internal issue 
tracker. When "Filed as internal issue USD-XXXX" is added to an issue, this is
an automated acknowledgment that the issue has been captured in our tracker and 
will be triaged for review. It does not mean work has started on the issue yet. 
Some GitHub issues may be tagged with labels following triage to invite 
contributions from the community. See the definitions of each label in GitHub 
`here <https://github.com/PixarAnimationStudios/OpenUSD/labels>`__.

.. _planning_major_changes:

********************
Making Major Changes 
********************

Please communicate your intent to make major changes with Pixar before starting 
work, to ensure your changes align with the OpenUSD strategy and reduce rework 
later. Below is the recommended workflow, with each step described in more detail 
afterwards.

.. image:: contributing_workflow_diagram.svg

Step 1. Get consensus for major changes
=======================================

If you would like to propose a major change that is not an architectural change, 
please give us a heads up by finding and commenting on the existing 
:ref:`GitHub issue <github_issues>` that represents the problem, or creating a 
new one if an appropriate issue doesn't already exist. Please briefly explain 
your high-level approach so the community and Pixar engineers can comment if 
there is already related work in progress or if the approach raises any concerns.

If you are proposing architectural changes such as schema changes/additions,
major C++ API changes/additions, or new build dependencies for OpenUSD, we 
recommend writing and posting a proposal to build consensus with the broader 
OpenUSD community. Proposals should be posted in the 
`OpenUSD-proposals GitHub repo <https://github.com/PixarAnimationStudios/OpenUSD-proposals>`__.
See the `repo README <https://github.com/PixarAnimationStudios/OpenUSD-proposals/blob/main/README.md>`__ 
for more details on the OpenUSD proposals process.

Proposals are often brought up and discussed in 
`ASWF USD Working Group meetings <https://www.aswf.io/meeting-calendar/>`__, 
which are typically held on Zoom every other Wednesday at 1pm PST.  

Proposals are ready to move forward when they've moved to the 
`Published <https://github.com/orgs/PixarAnimationStudios/projects/1/views/2>`__
phase. 

Step 2. Make code changes
=========================

Make code changes using the :ref:`git workflow <git_workflow>` described 
above, following the `Coding Guidelines <api/_page__coding__guidelines.html>`__.

Don't forget to 
`add API and user documentation <api/_page__coding__guidelines.html#Coding_Add_Documentation>`__
as needed. 

Where possible, break up your changes into smaller commits that are
functionally complete. This makes it easier for Pixar to review and, if 
necessary, to troubleshoot any regressions, when you submit your changes for 
review (Step 4 below). See 
`Make Small Atomic Changes <api/_page__coding__guidelines.html#Coding_Small_Changes>`__
for some approaches on splitting changes into separate commits. 

Step 3. Test code changes
=========================

Test your code changes, following the 
`Testing Guidelines <api/_page__testing__guidelines.html>`__ as needed.
With major changes, make sure to extend existing test coverage or provide new
tests for any added functionality.

Step 4. Submit code for review
==============================

Once your code is ready for review, submit a GitHub PR using the 
:ref:`pull request guidelines <pull_request_guidelines>`. 

For your PR description, begin with a brief summary of the change in 50 
characters or less, followed by a blank line and then a more detailed 
description of the change. Some questions to consider when drafting your  
description:

* How did this behave prior to your change?
* How was that behavior problematic?
* How does your change modify the behavior?
* What are the benefits of the modified behavior?
* Are there direct impacts to users?
* Are there follow up changes or asset changes required?

Be clear and concise. Include related PRs and issue identifiers, if 
applicable. **Please keep PR descriptions up-to-date with any code iterations.**

Pixar will do a code review of your pull request. Use GitHub PR review comments 
to discuss and review suggestions and questions that come up during the code 
review. Any code review comments will need to be either addressed or further 
discussed before the change can be merged. If reviewers use 
`GitHub suggested changes <https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/reviewing-changes-in-pull-requests/incorporating-feedback-in-your-pull-request>`__, 
you can use the web UI to automatically apply those changes if you agree with 
the suggestion. 

Code submitted for review is expected to be buildable and testable. There may be 
exceptions if you're actively engaged in discussions with a Pixar engineer about 
specific parts of code.

Step 5. Pixar will test and land your changes
=============================================

GitHub PRs are not landed directly into the GitHub dev branch, but rather into 
Pixar's internal development tree. We do this to facilitate the automated 
correctness and performance testing using production assets prior to merging the 
change. The open source branch is then extracted from Pixar's internal 
development tree and pushed to the OpenUSD GitHub **dev** branch on a 
:ref:`regular cadence <release_schedule>`. Once your PR has been incorporated
internally and the OpenUSD repo dev branch has been updated, Pixar will 
automatically close your PR.