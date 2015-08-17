---
output:
  md_document:
    variant: markdown_github
---

<!-- README.md is generated from README.Rmd. Please edit that file -->



# What is `binner`?
`binnr` is a package that creates, manages, and applies simple binning
transformations.

# Usage
The easiest way to use `binnr` is with the `bin.data` function. When applied to 
a `data.frame`, `bin.data` creates a `bin` object for every variable and stores
the information necessary to apply a weight-of-evidence (WoE) substitution. Why
is this beneficial? Data is often not well-behaved or continuous. Variables can 
have exception values, missing values, or monotonic relationships that need to
be enforced. `binnr` accomodates all of these situations and further enables the
modeler to tweak variable transformations to their liking.
