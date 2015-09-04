




## What is `binner`?
`binnr` is a package that creates, manages, and applies simple binning
transformations. It makes scorecard modeling easy and fast. Using `binnr`,
a modeler can discretize continuous variables, expand & collapse bins,
and apply monotonicity constraints all from within an easy-to-use
interface.

`binnr` not only discretizes variables and provides functions for 
manipulating them, but also performs weight-of-evidence substitution
on an a dataset transforming all predictors to the same scale and 
making them continuous. This has a number of benefits:

1. Continuous features can be used in more training algorithms
2. The WoE substitution replaces the input variable with its associated
log-odds -- ideal for modeling with logistic regression
3. Missing values are also substituted creating a data set of complete
cases

When paired with penalized regression techniques such as ridge or LASSO
regression, a model can be quickly created and fine-tuned to completion 
in a fraction of the time of traditional modeling techniques. All
of this with no loss (and often a gain) in predictive performance.

## Binning Algorithm

The binning algorithm used by `binnr` is completely writtin in `C` and 
is very fast. It uses a supervised disretization method based on 
information value to make recursive splits of the data. The algorithm
support monotonicity constraints, exception values, and missing values.

### Monotonicity

`binnr` supports 4 types of monotonicity within the `C` implementation.
Each type of constratint is specified by a special integer value.

| *Value* | *Meaning* |
|---------|-----------|
| 0 | No montonicity |
| -1 | Decreasing y as x increases |
| 1 | Increasing y as x increases |
| 2 | Either increasing or decreasing y as x increases |

Of special note is the value of 2. The algorithm implements this by 
making the first split in **any** direction and then uses that 
direction for the rest of the splits. This often results in the best
monotonic relationship without specifying the direction apriori.

## Overview

The basic workflow of building a scorecard with `binnr` is comprised of
a few basic steps:

1. Bin the dataset using the `bin` function
  * Use `summary` to see a high-level view of the binned data
  * Look for variables that should not be modeled
2. Perform the weight-of-evidence substition on the data
3. Fit a LASSO regression model
4. Use `adjust` on the final model variables to tweak them
5. Repeat steps 3-4 until satisfied

Each of these steps will be detailed further below with examples.

## Bin the data

A small dataset containig a variety of variable types is included with 
the `binnr` package. It consists of 891 passengers on the Titanic, their 
survival status, and several demographic and socioeconomic attributes.
This dataset will be used throught this help document.


```r
data(titanic)
head(titanic)
  Survived Pclass    Sex Age SibSp Parch    Fare Embarked
1        0      3   male  22     1     0  7.2500        S
2        1      1 female  38     1     0 71.2833        C
3        1      3 female  26     0     0  7.9250        S
4        1      1 female  35     1     0 53.1000        S
5        0      3   male  35     0     0  8.0500        S
6        0      3   male  NA     0     0  8.4583        Q
```

Binning the data is as simple as calling the `bin` function on a `data.frame`.


```r
bins <- bin(titanic[,-1], titanic$Survived)
```

This returns a `bin.list` object which contains a `bin` object for every
column of the dataset. Each `bin` object contains all of the information 
necessary to perform manipulations, plot data, and perform weight of 
evidence substitutions. Printing a `bin.list` object prints a summary of 
what it contains.


```r
bins
binnr bin.list object
  |--   7 Bins
  |--   3 Discrete
  |--   4 Continuous
```

There are 7 bins contained within the `bin.list` object - 3 discreted and 4
continuous. The distinction between discrete and continuous bins will be
demonstrated when using the `adjust` function.

Because `bin.list` is a list underneath, individual bins can be accessed 
in the normal list indexing manner. Printing a single bin produces a WoE
table with detailed statistics about the binned and target variables


```r
bins$Pclass

IV: 0.50095 | Variable: Pclass
       #0  #1   N    %0    %1  P(1)    WoE      IV
 1. 1  80 136 216 0.146 0.398 0.630  1.004 0.25293
 2. 2  97  87 184 0.177 0.254 0.473  0.364 0.02832
 3. 3 372 119 491 0.678 0.348 0.242 -0.666 0.21970
Total 549 342 891 1.000 1.000 0.384  0.000 0.50095
```

Furthermore, a `bin.list` may also be subset just like a base R list:


```r
bins[1:4]
binnr bin.list object
  |--   4 Bins
  |--   2 Discrete
  |--   2 Continuous
```

## Apply Weight-of-Evidence Substitutions

`binnr` provides a `predict` function that is used to perform the WoE 
substitution on a `data.frame`. The columns are matched by name and a
matrix of numeric values is returned.


```r
binned <- predict(bins, titanic)
```


```r
head(binned)
      Pclass        Sex        Age      SibSp      Parch       Fare    Embarked
1 -0.6664827 -0.9838327 -0.1027299  0.6170756 -0.1737481 -0.8484681 -0.20359896
2  1.0039160  1.5298770 -0.1148437  0.6170756 -0.1737481  0.7327989  0.68839908
3 -0.6664827  1.5298770 -0.1027299 -0.1660568 -0.1737481 -0.9238176 -0.20359896
4  1.0039160  1.5298770  0.5805232  0.6170756 -0.1737481  0.7327989 -0.20359896
5 -0.6664827 -0.9838327  0.5805232 -0.1660568 -0.1737481 -0.9238176 -0.20359896
6 -0.6664827 -0.9838327  0.0000000 -0.1660568 -0.1737481 -0.9238176  0.02433748
```


Creating a table of the WoE-substituted values with the original values
illustrates what `binnr` is doing behind the scenes:


```r
table(titanic$Pclass, round(binned[,'Pclass'], 3))
   
    -0.666 0.364 1.004
  1      0     0   216
  2      0   184     0
  3    491     0     0
```
We can verify that values of `male` are being coded correctly to the value found
in the WoE table. The same holds true for `female`.

### Logistic Regression

Once the variable transoformations have been applied, a logistic regression
model may be fit. We will be applying a new logistic regression algorithm called
`LASSO`. It fits the model and performs variable selection at the same time.
More about LASSO regression can be found [here](http://statweb.stanford.edu/~tibs/lasso.html).

LASSO regression requires that we specify a penalty argument to constrain the 
coefficients. We will be using cross-validation to determine this parameter
automatically. Furthermore, since our variables are already transformed the way
we like, we will also force the parameters to be greater than zero. This will
prevent any "flips" from occuring in our final model.

And here is the raw variable crossed with the transformed variable:

```
Loading required package: Matrix
Loading required package: foreach
foreach: simple, scalable parallel programming from Revolution Analytics
Use Revolution R for scalability, fault tolerance and more.
http://www.revolutionanalytics.com
Loaded glmnet 2.0-2
```


```r
fit <- cv.glmnet(binned, titanic$Survived, alpha=1, family="binomial",
                 nfolds = 5, lower.limits=0)
plot(fit)
```

![plot of chunk unnamed-chunk-11](plots/README-unnamed-chunk-11-1.png) 

The resulting plot shows the error on the y-axis and the penalty term on the
x-axis. The penalty term controls the size of the coefficients and how many of
them are not equal to zero. The first dashed line represents the size of the
penalty term that has the lowest cross-validation error. We can access this
value easily by using the "lambda.min" argument where appropriate. For example, 
to find the optimal coefficients:


```r
coef(fit, s="lambda.min")
8 x 1 sparse Matrix of class "dgCMatrix"
                     1
(Intercept) -0.4699424
Pclass       0.8618862
Sex          0.9899909
Age          1.1595609
SibSp        0.4288312
Parch        .        
Fare         0.2607622
Embarked     0.5080654
```


