# Semi-automatic Dependency Updates with Dependabot

This document describes how we automatically keep the dependencies in `cpp-samples` up to date. The intended audience
are developers and maintainers of `cpp-samples`.

## Problem Statement

The samples in `cpp-samples` are not under active development. Typically they are created to demonstrate how to use
some functionality in Google Cloud Platform, or how to demo some interesting use-case for the C++ client libraries. Once
created, the original developers may not revisit the sample or demo for months or years. In that time the sample may
become stale. Particularly, the dependencies for that sample maybe change, and make the sample unusable.

## Background

It is possible to keep the samples "working" by pinning all the dependencies to the version at the time the sample is
written, this may satisfy the compiler, but does not provide customers with a good experience when they try to use
the sample or demo in their code.

We could manually revisit all the samples and demos, this is error-prone and tedious.

We could run nightly builds that compile everything against "latest" and force us to update as-needed. This creates
more work than probably we need to, and it makes the builds more flaky than necessary.

Most of the samples and demos use [`vcpkg`](https:///github.com/microsoft/vcpkg) to fetch and compile their
dependencies. Some demos also use containers where all the dependencies are obtained via the native package manager.

[Dependabot] is a GitHub product to automatically update your dependencies. It can be configured to periodically examine
your repository and create pull requests if any dependency needs to be updated. `Dependabot` does not support `vcpkg` or
handle arbitrary GitHub repositories referenced from dockerfiles or scripts.

## Overview

The community has developed a [workaround][workaround-link] to handle these cases. At a high-level, this workaround is:

* Define your dependencies as steps in a GitHub action.
    * This action is only a placeholder, it is only enabled for a branch that does not exist.
* `dependabot` will create a PR to update these steps.
* The human reviewer for this PR needs to update the corresponding scripts.
    * Humans can edit the automatically generated PR, so all changes take place in a single PR.
    
This is the workflow we have adopted for `cpp-samples`.

[Dependabot]:  https://dependabot.com/
[workaround-link]: https://github.com/agilepathway/label-checker/blob/master/.github/DEPENDENCIES.md#workaround-for-other-dependencies
