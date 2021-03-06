% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/classing_class.R
\docType{methods}
\name{create_bin}
\alias{create_bin}
\alias{create_bin,numeric-method}
\alias{create_bin,factor-method}
\alias{create_bin,character-method}
\alias{create_bin,ANY-method}
\title{generic method for create_bin}
\usage{
create_bin(x, ...)

\S4method{create_bin}{numeric}(x, ...)

\S4method{create_bin}{factor}(x, ...)

\S4method{create_bin}{character}(x, ...)

\S4method{create_bin}{ANY}(x, ...)
}
\arguments{
\item{x}{the varibale used for the Bins \code{x} value}

\item{...}{additional arguments passed on to the Bin constructors}
}
\value{
a Continuous object

a Discrete object
}
\description{
generic method for create_bin
}
\section{Methods (by class)}{
\itemize{
\item \code{numeric}: wrap variable in Continuous object

\item \code{factor}: wrap variable in Discrete object

\item \code{character}: create_bin fallback warning user

\item \code{ANY}: create_bin fallback warning user
}}

