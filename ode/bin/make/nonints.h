/*
 * Distributed as part of the Mach Operating System
 */
/*
 * @OSF_FREE_COPYRIGHT@
 * 
 * Copyright (c) 1990, 1991
 * Open Software Foundation, Inc.
 * 
 * Permission is hereby granted to use, copy, modify and freely distribute
 * the software in this file and its documentation for any purpose without
 * fee, provided that the above copyright notice appears in all copies and
 * that both the copyright notice and this permission notice appear in
 * supporting documentation.  Further, provided that the name of Open
 * Software Foundation, Inc. ("OSF") not be used in advertising or
 * publicity pertaining to distribution of the software without prior
 * written permission from OSF.  OSF makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */
/*
 * HISTORY
 * $Log:	nonints.h,v $
 * Revision 2.2  92/05/20  20:14:15  mrt
 * 	First checkin
 * 	[92/05/20  16:31:26  mrt]
 * 
 */
char **brk_string(), *emalloc(), *str_concat();
char *strdup();

ReturnStatus	Arch_ParseArchive ();
void	Arch_Touch ();
void	Arch_TouchLib ();
int	Arch_MTime ();
int	Arch_MemMTime ();
void	Arch_FindLib ();
Boolean	Arch_LibOODate ();
void	Arch_Init ();
void	Compat_Run();
void	Dir_Init ();
Boolean	Dir_HasWildcards ();
void	Dir_Expand ();
char *	Dir_FindFile ();
int	Dir_MTime ();
void	Dir_AddDir ();
ClientData	Dir_CopyDir ();
char *	Dir_MakeFlags ();
void	Dir_Destroy ();
void	Dir_ClearPath ();
void	Dir_Concat ();
int	Make_TimeStamp ();
Boolean	Make_OODate ();
int	Make_HandleUse ();
void	Make_Update ();
void	Make_DoAllVar ();
Boolean	Make_Run ();
void	Job_Touch ();
Boolean	Job_CheckCommands ();
void	Job_CatchChildren ();
void	Job_CatchOutput ();
void	Job_Make ();
void	Job_Init ();
Boolean	Job_Full ();
Boolean	Job_Empty ();
ReturnStatus	Job_ParseShell ();
int	Job_End ();
void	Job_Wait();
void	Job_AbortAll ();
void	Main_ParseArgLine ();
void	Error ();
void	Fatal ();
void	Punt ();
void	DieHorribly ();
void	Finish ();
void	Parse_Error ();
Boolean	Parse_IsVar ();
void	Parse_DoVar ();
void	Parse_AddIncludeDir ();
void	Parse_File();
Lst	Parse_MainName();
void	Suff_ClearSuffixes ();
Boolean	Suff_IsTransform ();
GNode *	Suff_AddTransform ();
void	Suff_AddSuffix ();
int	Suff_EndTransform ();
Lst	Suff_GetPath ();
void	Suff_DoPaths();
void	Suff_AddInclude ();
void	Suff_AddLib ();
void	Suff_FindDeps ();
void	Suff_SetNull();
void	Suff_Init ();
void	Targ_Init ();
GNode *	Targ_NewGN ();
GNode *	Targ_FindNode ();
Lst	Targ_FindList ();
Boolean	Targ_Ignore ();
Boolean	Targ_Silent ();
Boolean	Targ_Precious ();
void	Targ_SetMain ();
int	Targ_PrintCmd ();
char *	Targ_FmtTime ();
void	Targ_PrintType ();
char *	Str_Concat ();
int	Str_Match();
void	Var_Delete();
void	Var_Set ();
void	Var_Append ();
Boolean	Var_Exists();
char *	Var_Value ();
char *	Var_Parse ();
char *	Var_Skip ();
char *	Var_Subst ();
char *	Var_GetTail();
char *	Var_GetHead();
void	Var_Init ();
char *	Str_FindSubstring();
