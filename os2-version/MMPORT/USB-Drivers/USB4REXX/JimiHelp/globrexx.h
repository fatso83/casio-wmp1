
// ============================================================================
#define REXX_INVALIDCALL        40
#define REXX_VALIDCALL           0

BOOL  REXX_SetVariable (PSZ VarName, PCHAR StringPtr, ULONG StringLen);
BOOL  REXX_SetVariableViaPSZ (PSZ VarName, PSZ PSZPtr);
BOOL  REXX_SetVariableViaULONG (PSZ VarName, ULONG Value);
BOOL  REXX_SetVariableViaULONGHex (PSZ VarName, ULONG Value);
LONG  REXX_GetStemVariable (PCHAR DestPtr, ULONG DestMaxLength, PSZ VarNameBase, USHORT VarNameID);
BOOL  REXX_SetStemVariable (PSZ VarNameBase, USHORT VarNameID, PCHAR StringPtr, ULONG StringLen);
BOOL  REXX_SetStemVariableViaPSZ (PSZ VarNameBase, USHORT VarNameID, PSZ PSZPtr);

#ifdef INCL_REXX_STDLOADDROP
// ============================================================================
ULONG APIENTRY REXX_MyLoadFuncs (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   INT FunctionCount = 0;
   INT CurNo         = 0;

   if (numargs > 1)
      return REXX_INVALIDCALL;

   /* check, if quietarguments */
   if ((numargs==0) || (args[0].strlength!=5) || (strncmp(args[0].strptr,"quiet",5)!=0))
      printf(REXX_WelcomeMessage);

   FunctionCount = sizeof(REXX_FunctionTable)/sizeof(PSZ);
   while (CurNo<FunctionCount) {
      RexxRegisterFunctionDll(REXX_FunctionTable[CurNo], REXX_MyDLLName, REXX_FunctionTable[CurNo]);
      CurNo++;
    }

   retstr->strlength = 0;                   // No Result
   return REXX_VALIDCALL;
 }

// ============================================================================
ULONG APIENTRY REXX_MyDropFuncs (PUCHAR name, ULONG numargs, RXSTRING args[], PSZ queuename, PRXSTRING retstr) {
   INT FunctionCount = 0;
   INT CurNo         = 0;

   if (numargs != 0)
      return REXX_INVALIDCALL;

   FunctionCount = sizeof(REXX_FunctionTable)/sizeof(PSZ);
   while (CurNo<FunctionCount) {
      RexxDeregisterFunction(REXX_FunctionTable[CurNo]);
      CurNo++;
    }

   retstr->strlength = 0;                   // No Result
   return REXX_VALIDCALL;                   // no error on call
 }
#endif
