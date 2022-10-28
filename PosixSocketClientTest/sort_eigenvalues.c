////////////////////////////////////////////////////////////////////////////////
// File: sort_eigenvalues.c                                                   //
// Routines:                                                                  //
//    Sort_Eigenvalues                                                        //
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//  void Sort_Eigenvalues(double eigenvalues[], double *eigenvectors, int n,  //
//                                                            int sort_order) //
//                                                                            //
//  Description:                                                              //
//     Sort the eigenvalues and corresponding eigenvectors. If sort_order is  //
//     positive, then the eigenvalues are sorted in ascending order and if    //
//     sort_order is negative, then the eigenvalues are sorted in descending  //
//     order.  The columns of the matrix eigenvectors are permuted so that    //
//     the ith eigenvalue has corresponding eigenvector in the ith column of  //
//     the matrix eigenvectors.                                               //
//                                                                            //
//  Arguments:                                                                //
//     double  eigenvalues  - Array, dimension n, containing the eigenvalues. //
//     double* eigenvectors - Matrix of eigenvectors, the ith column of which //
//                            contains an eigenvector corresponding to the    //
//                            ith eigenvalue in the array eigenvalues.        //
//     int     n            - The dimension of the array eigenvalues and the  //
//                            number of columns and rows of the matrix        //
//                            eigenvectors.                                   //
//     int     sort_order   - An indicator used to specify the order in which //
//                            the eigenvalues and eigenvectors are to be      //
//                            returned.  If sort_order > 0, then the          //
//                            eigenvalues are sorted in increasing order.     //
//                            If sort_order < 0, then the eigenvalues are     //
//                            sorted in decreasing order.  If sort_order = 0, //
//                            then the eigenvalues are returned in the order, //
//                            found.                                          //
//                                                                            //
//  Example:                                                                  //
//     #define N                                                              //
//     double double eigenvalues[N], double eigenvectors[N][N];               //
//     int sort_order = 1;                                                    //
//                                                                            //
//     (your code to calculate the eigenvalues and eigenvectors)              //
//                                                                            //
//     Sort_Eigenvalues(eigenvalues, (double*)eigenvectors, N,  sort_order);  //
//     printf(" The Eigenvalues are: \n"); ...                                //
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
void Sort_Eigenvalues(double eigenvalues[], double *eigenvectors, int n,
                                                                int sort_order)
{
   int i,j;
   int m;
   double x, *pm, *pi;

   if (sort_order == 0) return;
   if (sort_order < 0) {
      for (i = 0; i < (n - 1); i++) {
         m = i;
         for (j = i+1; j < n; j++) if (eigenvalues[m] < eigenvalues[j]) m = j;
         if (m == i) continue;
         x = eigenvalues[m];
         eigenvalues[m] = eigenvalues[i];
         eigenvalues[i] = x;
         pm = eigenvectors + m;
         pi = eigenvectors + i;
         for (j = 0; j < n; pm += n, pi += n, j++) {
            x = *pm;
            *pm = *pi;
            *pi = x;
         }
      }
   }
   else {
      for (i = 0; i < (n - 1); i++) {
         m = i;
         for (j = i+1; j < n; j++) if (eigenvalues[m] > eigenvalues[j]) m = j;
         if (m == i) continue;
         x = eigenvalues[m];
         eigenvalues[m] = eigenvalues[i];
         eigenvalues[i] = x;
         pm = eigenvectors + m;
         pi = eigenvectors + i;
         for (j = 0; j < n; pm += n, pi += n, j++) {
            x = *pm;
            *pm = *pi;
            *pi = x;
         }
      }
   }
   return;
}


void Sort_Eigenvalues_Transpose(double eigenvalues[], double *eigenvectors,
                                                        int n, int sort_order)
{
   int i,j;
   int m;
   double x, *pm, *pi;

   if (sort_order == 0) return;
   if (sort_order < 0) {
      for (i = 0; i < (n - 1); i++) {
         m = i;
         for (j = i+1; j < n; j++) if (eigenvalues[m] < eigenvalues[j]) m = j;
         if (m == i) continue;
         x = eigenvalues[m];
         eigenvalues[m] = eigenvalues[i];
         eigenvalues[i] = x;
         pm = eigenvectors + m*n;
         pi = eigenvectors + i*n;
         for (j = 0; j < n; pm++, pi++, j++) {
            x = *pm;
            *pm = *pi;
            *pi = x;
         }
      }
   }
   else {
      for (i = 0; i < (n - 1); i++) {
         m = i;
         for (j = i+1; j < n; j++) if (eigenvalues[m] > eigenvalues[j]) m = j;
         if (m == i) continue;
         x = eigenvalues[m];
         eigenvalues[m] = eigenvalues[i];
         eigenvalues[i] = x;
         pm = eigenvectors + m*n;
         pi = eigenvectors + i*n;
         for (j = 0; j < n; pm++, pi++, j++) {
            x = *pm;
            *pm = *pi;
            *pi = x;
         }
      }
   }
   return;
}

