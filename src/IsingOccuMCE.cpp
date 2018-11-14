#include <Rcpp.h>
using namespace Rcpp;

// This is a simple example of exporting a C++ function to R. You can
// source this function into an R session using the Rcpp::sourceCpp
// function (or via the Source button on the editor toolbar). Learn
// more about Rcpp at:
//
//   http://www.rcpp.org/
//   http://adv-r.had.co.nz/Rcpp.html
//   http://gallery.rcpp.org/
//

// Inner functions
// this from R package IsingSampler
// This is MH method
double Pplus(int i, NumericMatrix J, IntegerVector s, NumericVector h, double beta, IntegerVector responses, NumericMatrix detPi)
{
  // The function computes the probability that node i is in Response 1 instead of 0, given all other nodes, which might be missing.
  // Output: minimal and maximal probablity
  
  double H0 = h[i] * responses[0]; // relevant part of the Hamiltonian for state = 0
  double H1 = h[i] * responses[1]; // relevant part of the Hamiltonian for state = 1

  //double Res;

  
  int N = J.nrow();
  
  for (int j=0; j<N; j++)
  {
    if (i != j)
    {
       H0 += J(i,j) * responses[0] * s[j];
       H1 += J(i,j) * responses[1] * s[j];
    }
  }
  
  return((exp(beta * H1) * detPi[1]) / ( exp(beta * H0) * detPi[0] + exp(beta * H1) * detPi[1] ));// MH ratio here, need changing
}

IntegerVector IsingMet(NumericMatrix graph, NumericVector thresholds, double beta, int nIter, IntegerVector responses,
IntegerVector constrain, NumericMatrix detP)
{
  // Parameters and results vector:
  int N = graph.nrow();
  IntegerVector state =  ifelse(runif(N) < 0.5, responses[1], responses[0]);
  for (int i=0; i<N; i++)
  {
    if (constrain[i] != INT_MIN)
    {
      state[i] = constrain[i];
    }
  }
  double u;
  double P;
    
    // START ALGORITHM
    for (int it=0;it<nIter;it++)
    {
      for (int node=0;node<N;node++)
      {
        if (constrain[node] == INT_MIN)
        {
         u = runif(1)[0];
		 detPi = detP(node,_) // detection probability given state for site node
         P = Pplus(node, graph, state, thresholds, beta, responses, detPi);
          if (u < P)
         {
           state[node] = responses[1];
         } else 
         {
           state[node] = responses[0];
         } 
        }
      }
    }
   
  return(state);
}



// FUNCTIONS FOR EXACT SAMPLING //

// Inner function to resize list:
List resize( const List& x, int n ){
    int oldsize = x.size() ;
    List y(n) ;
    for( int i=0; i<oldsize; i++) y[i] = x[i] ;
    return y ;
}

// Inner function to simulate random uniforms in a matrix:
NumericMatrix RandMat(int nrow, int ncol)
 {
  int N = nrow * ncol;
  NumericMatrix Res(nrow,ncol);
  NumericVector Rands  = runif(N);
   for (int i = 0; i < N; i++) 
  {
    Res[i] = Rands[i];
  }
  return(Res);
 }

// Computes maximal and minimal probability of node flipping:
NumericVector PplusMinMax(int i, NumericMatrix J, IntegerVector s, NumericVector h, double beta, IntegerVector responses, NumericMatrix detPi)
{
  // The function computes the probability that node i is in Response 1 instead of 0, given all other nodes, which might be missing.
  // Output: minimal and maximal probablity
  
  NumericVector H0(2, h[i] * responses[0]); // relevant part of the Hamiltonian for state = 0
  NumericVector H1(2, h[i] * responses[1]); // relevant part of the Hamiltonian for state = 1
  
  NumericVector Res(2);
  
  int N = J.nrow();
  NumericVector TwoOpts(2);
  
  for (int j=0; j<N; j++)
  {
    if (i != j)
    {
      if (s[j] != INT_MIN)
      {
       H0[0] += J(i,j) * responses[0] * s[j];
       H0[1] += J(i,j) * responses[0] * s[j];
       H1[0] += J(i,j) * responses[1] * s[j];
       H1[1] += J(i,j) * responses[1] * s[j]; 
      } else 
      {
               
        TwoOpts[0] = J(i,j) * responses[1] * responses[0];
        TwoOpts[1] = J(i,j) * responses[1] * responses[1];

        if (TwoOpts[1] > TwoOpts[0])
        {
          H1[0] += TwoOpts[0];
          H1[1] += TwoOpts[1];
          
          H0[0] += J(i,j) * responses[0] * responses[0];
          H0[1] += J(i,j) * responses[0] * responses[1];
        } else 
        {
          H1[0] += TwoOpts[1];
          H1[1] += TwoOpts[0];          
          
          H0[0] += J(i,j) * responses[0] * responses[1];
          H0[1] += J(i,j) * responses[0] * responses[0];
        }
      }
    }
  }

  Res[0] = (exp(beta * H1[0]) * detPi[1]) / ( exp(beta * H0[0]) * detPi[0] + exp(beta * H1[0]) * detPi[1] );
  Res[1] = (exp(beta * H1[1]) * detPi[1]) / ( exp(beta * H0[1]) * detPi[0] + exp(beta * H1[1]) * detPi[1] );
  
  
  return(Res);
}
       
// Inner function:
IntegerVector IsingEx(NumericMatrix graph, NumericVector thresholds, double beta, int nIter, IntegerVector responses, bool exact,
IntegerVector constrain, NumericMatrix detP)
{
  // Parameters and results vector:
  int N = graph.nrow();
  IntegerVector state(N, INT_MIN);
  double u;
  NumericVector P(2);
  int maxChain = 100;
  List U(1);
  int minT = 0;
  bool anyNA = true;
    
  do
  { 
    // Resize U if needed:
    if (minT > 0)
    {
      U = resize(U, minT+1);
    }
    
    // Generate new random numbers:
    U[minT] = RandMat(nIter, N);
    
    // Initialize states:
    for (int i=0; i<N; i++)
    {
      if (exact)
      {
        state[i] = INT_MIN;
      } else 
      {
        state[i] = ifelse(runif(1) < 0.5, responses[1], responses[0])[0];
      }
    }    

    // START ALGORITHM
    for (int t=minT; t > -1;  t--)
    {
      for (int it=0;it<nIter;it++)
      {
        NumericMatrix Ucur = U[t];
        for (int node=0;node<N;node++)
        {
		  detPi = detP(node,_)
          u = Ucur(it, node);
          P = PplusMinMax(node, graph, state, thresholds, beta, responses, detPi);
          if (u < P[0])
          {
            state[node] = responses[1];
          } else if (u >= P[1])
          {
            state[node] = responses[0];
          } else 
          {
            state[node] = INT_MIN;
          }
        }
      }
    }
    
    anyNA = false;
    if (exact)
    {
      if (minT < maxChain)
      {
       for (int i=0; i<N; i++)
       {
        if (state[i] == INT_MIN)
        {
          anyNA = true;
        }
        } 
      } 
    }    
    minT++;
    
  } while (anyNA);

  // Rf_PrintValue(wrap(minT));
  return(state);
}






// [[Rcpp::export]]
IntegerMatrix IsingSamplerCpp(int n, NumericMatrix graph, NumericVector thresholds, double beta, int nIter, IntegerVector responses, bool exact,
IntegerMatrix constrain, NumericMatrix detP)
{
  int Ni = graph.nrow();
  IntegerMatrix Res(n,Ni);
  IntegerVector state(Ni);
  IntegerVector constrainVec(Ni);
  if (exact)
  {
    for (int s=0;s<n;s++)
    {
      for (int i=0;i<Ni;i++) constrainVec[i] = constrain(s,i);
      state = IsingEx(graph, thresholds, beta, nIter, responses, exact, constrainVec, detP);
      for (int i=0;i<Ni;i++) Res(s,i) = state[i];
    }
  } else 
  {
    for (int s=0;s<n;s++)
    {
      for (int i=0;i<Ni;i++) constrainVec[i] = constrain(s,i);
      state = IsingMet(graph, thresholds, beta, nIter, responses, constrainVec, detP);
      for (int i=0;i<Ni;i++) Res(s,i) = state[i];
    }
  }
  
  return(Res);
}








// You can include R code blocks in C++ files processed with sourceCpp
// (useful for testing and development). The R code will be automatically
// run after the compilation.
//


