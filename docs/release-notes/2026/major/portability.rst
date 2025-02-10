Portability
^^^^^^^^^^^

.. Note to developers!
   Please use """"""" to underline the individual entries for fixed issues in the subfolders,
   otherwise the formatting on the webpage is messed up.
   Also, please use the syntax :issue:`number` to reference issues on GitLab, without
   a space between the colon and number!

MPI 3.0 is now required, 2.0 no longer supported
""""""""""""""""""""""""""""""""""""""""""""""""

When building with MPI library, |Gromacs| 2026 requires it to be conformant with MPI 3.0.
This should not be a problem as long as you are using MPI library from the past decade
(e.g., OpenMPI 1.8+, MPICH 3.0+).


:issue:`4933`

