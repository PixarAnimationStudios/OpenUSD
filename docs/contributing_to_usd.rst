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
<https://github.com/PixarAnimationStudios/USD>`_ you can find the two possible
CLAs:

    #. `USD_CLA_Corporate.pdf
       <https://github.com/PixarAnimationStudios/USD/blob/release/USD_CLA_Corporate.pdf>`_
       : please sign this one for corporate use

    #. `USD_CLA_Individual.pdf
       <https://github.com/PixarAnimationStudios/USD/blob/release/USD_CLA_Individual.pdf>`_
       : please sign this one if you're an individual contributor

Once your CLA is signed, send it to `usd-cla@pixar.com
<mailto:usd-cla@pixar.com>`_ (please make sure to include your github username)
and wait for confirmation that we've received it. After that, you can submit
pull requests.

Supplemental Terms
******************

By and in consideration for using a Pixar site (e.g., Pixar's USD-proposals
site), providing Submissions to Pixar, or by clicking a box that states that
you accept or agree to these terms, you signify your agreement to these
Supplemental Terms.

You hereby grant to Pixar and our licensees, distributors, agents,
representatives and other authorized users, a perpetual, non-exclusive,
irrevocable, fully-paid, royalty-free, sub-licensable and transferable
(in whole or part) worldwide license under all copyrights, trademarks, patents,
trade secret rights, data rights, privacy and publicity rights and other
proprietary rights you or your affiliates now or hereafter own or control, to
reproduce, transmit, display, exhibit, distribute, index, comment on, perform,
create derivative works based upon, modify, make, use, sell, have made, import,
and otherwise exploit in any manner, the Submissions and any implementations of
the Submissions, in whole or in part, including in all media formats and
channels now known or hereafter devised, for any and all purposes, including
entertainment, research, news, advertising, promotional, marketing, publicity,
trade or commercial purposes, all without further notice to you, with or without
attribution, and without the requirement of any permission from or payment to
you or any other person or entity.  

As used herein, Submissions shall mean any text, proposals, white papers,
specifications, messages, technologies, ideas, concepts, pitches, suggestions,
stories, screenplays, treatments, formats, artwork, photographs, drawings,
videos, audiovisual works, musical compositions (including lyrics), sound
recordings, characterizations, software code, algorithms, structures, your
and/or other persons' names, likenesses, voices, usernames, profiles, actions,
appearances, performances and/or other biographical information or material,
and/or other similar materials that you submit, post, upload, embed, display,
communicate or otherwise distribute to or for Pixar or a Pixar site.

These Supplemental Terms are in addition to any prior or contemporaneous
agreement (and shall remain in effect notwithstanding any future agreement)
that you may have with Pixar.

At any time, we may update these Supplemental Terms (including by modification,
deletion and/or addition of any portion thereof).  Any update to these
Supplemental Terms will be effective thirty (30) calendar days following our
posting of the updated Supplemental Terms to a Pixar site.

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
      submitting a pull request.

    * We highly recommend posting issues on GitHub for features or bug fixes
      that you intend to work on before beginning any development. That way, if
      someone else is working on a similar project, you can collaborate, or you
      can get early feedback which can sometimes save time.

    * Please make sure that your pull requests are clean. Use the rebase and
      squash git facilities as needed to ensure that the pull request is as
      clean as possible.

    * Please make pull requests that are small and atomic - in general, it is
      easier for us to merge pull requests that are small and serve a single
      purpose than those that are sweeping and combine several functional pieces
      in a single PR.

Git Workflow
************

Here is the workflow we recommend for working on USD if you intend on
contributing changes back:

    #. Post an issue on github to let folks know about the feature or bug that
       you found, and mention that you intend to work on it. That way, if
       someone else is working on a similar project, you can collaborate, or you
       can get early feedback which can sometimes save time.

        .. | space |

    #. Use the github website to fork your own private repository.

        .. | space |

    #. Clone your fork to your local machine, like this:

       .. code-block:: sh

          git clone https://github.com/you/USD.git

       

    #. Add Pixar's USD repo as upstream to make it easier to update your remote
       and local repos with the latest changes:

       .. code-block:: sh

          cd USD
          git remote add upstream https://github.com/PixarAnimationStudios/USD.git

       

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

