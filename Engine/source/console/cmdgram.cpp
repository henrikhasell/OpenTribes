/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         CMDparse
#define yylex           CMDlex
#define yyerror         CMDerror
#define yydebug         CMDdebug
#define yynerrs         CMDnerrs
#define yylval          CMDlval
#define yychar          CMDchar

/* First part of user prologue.  */
#line 1 "CMDgram.y"


// bison --defines=cmdgram.h --verbose -o cmdgram.cpp -p CMD CMDgram.y

// Make sure we don't get gram.h twice.
#define _CMDGRAM_H_

#include <stdlib.h>
#include <stdio.h>
#include "console/console.h"
#include "console/compiler.h"
#include "console/consoleInternal.h"
#include "core/strings/stringFunctions.h"

#ifndef YYDEBUG
#define YYDEBUG 0
#endif

#define YYSSIZE 350

int outtext(char *fmt, ...);
extern int serrors;

#define nil 0
#undef YY_ARGS
#define YY_ARGS(x)   x

int CMDlex();
void CMDerror(char *, ...);

#ifdef alloca
#undef alloca
#endif
#define alloca dMalloc

template< typename T >
struct Token
{
   T value;
   U32 lineNumber;
};

#line 44 "CMDgram.y"

        /* Reserved Word Definitions */
#line 55 "CMDgram.y"

        /* Constants and Identifier Definitions */
#line 69 "CMDgram.y"

        /* Operator Definitions */

#line 131 "cmdgram.cpp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "cmdgram.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_rwDEFINE = 3,                   /* rwDEFINE  */
  YYSYMBOL_rwENDDEF = 4,                   /* rwENDDEF  */
  YYSYMBOL_rwDECLARE = 5,                  /* rwDECLARE  */
  YYSYMBOL_rwDECLARESINGLETON = 6,         /* rwDECLARESINGLETON  */
  YYSYMBOL_rwBREAK = 7,                    /* rwBREAK  */
  YYSYMBOL_rwELSE = 8,                     /* rwELSE  */
  YYSYMBOL_rwCONTINUE = 9,                 /* rwCONTINUE  */
  YYSYMBOL_rwGLOBAL = 10,                  /* rwGLOBAL  */
  YYSYMBOL_rwIF = 11,                      /* rwIF  */
  YYSYMBOL_rwNIL = 12,                     /* rwNIL  */
  YYSYMBOL_rwRETURN = 13,                  /* rwRETURN  */
  YYSYMBOL_rwWHILE = 14,                   /* rwWHILE  */
  YYSYMBOL_rwDO = 15,                      /* rwDO  */
  YYSYMBOL_rwENDIF = 16,                   /* rwENDIF  */
  YYSYMBOL_rwENDWHILE = 17,                /* rwENDWHILE  */
  YYSYMBOL_rwENDFOR = 18,                  /* rwENDFOR  */
  YYSYMBOL_rwDEFAULT = 19,                 /* rwDEFAULT  */
  YYSYMBOL_rwFOR = 20,                     /* rwFOR  */
  YYSYMBOL_rwFOREACH = 21,                 /* rwFOREACH  */
  YYSYMBOL_rwFOREACHSTR = 22,              /* rwFOREACHSTR  */
  YYSYMBOL_rwIN = 23,                      /* rwIN  */
  YYSYMBOL_rwDATABLOCK = 24,               /* rwDATABLOCK  */
  YYSYMBOL_rwSWITCH = 25,                  /* rwSWITCH  */
  YYSYMBOL_rwCASE = 26,                    /* rwCASE  */
  YYSYMBOL_rwSWITCHSTR = 27,               /* rwSWITCHSTR  */
  YYSYMBOL_rwCASEOR = 28,                  /* rwCASEOR  */
  YYSYMBOL_rwPACKAGE = 29,                 /* rwPACKAGE  */
  YYSYMBOL_rwNAMESPACE = 30,               /* rwNAMESPACE  */
  YYSYMBOL_rwCLASS = 31,                   /* rwCLASS  */
  YYSYMBOL_rwASSERT = 32,                  /* rwASSERT  */
  YYSYMBOL_ILLEGAL_TOKEN = 33,             /* ILLEGAL_TOKEN  */
  YYSYMBOL_CHRCONST = 34,                  /* CHRCONST  */
  YYSYMBOL_INTCONST = 35,                  /* INTCONST  */
  YYSYMBOL_TTAG = 36,                      /* TTAG  */
  YYSYMBOL_VAR = 37,                       /* VAR  */
  YYSYMBOL_IDENT = 38,                     /* IDENT  */
  YYSYMBOL_TYPEIDENT = 39,                 /* TYPEIDENT  */
  YYSYMBOL_DOCBLOCK = 40,                  /* DOCBLOCK  */
  YYSYMBOL_STRATOM = 41,                   /* STRATOM  */
  YYSYMBOL_TAGATOM = 42,                   /* TAGATOM  */
  YYSYMBOL_FLTCONST = 43,                  /* FLTCONST  */
  YYSYMBOL_44_ = 44,                       /* '+'  */
  YYSYMBOL_45_ = 45,                       /* '-'  */
  YYSYMBOL_46_ = 46,                       /* '*'  */
  YYSYMBOL_47_ = 47,                       /* '/'  */
  YYSYMBOL_48_ = 48,                       /* '<'  */
  YYSYMBOL_49_ = 49,                       /* '>'  */
  YYSYMBOL_50_ = 50,                       /* '='  */
  YYSYMBOL_51_ = 51,                       /* '.'  */
  YYSYMBOL_52_ = 52,                       /* '|'  */
  YYSYMBOL_53_ = 53,                       /* '&'  */
  YYSYMBOL_54_ = 54,                       /* '%'  */
  YYSYMBOL_55_ = 55,                       /* '('  */
  YYSYMBOL_56_ = 56,                       /* ')'  */
  YYSYMBOL_57_ = 57,                       /* ','  */
  YYSYMBOL_58_ = 58,                       /* ':'  */
  YYSYMBOL_59_ = 59,                       /* ';'  */
  YYSYMBOL_60_ = 60,                       /* '{'  */
  YYSYMBOL_61_ = 61,                       /* '}'  */
  YYSYMBOL_62_ = 62,                       /* '^'  */
  YYSYMBOL_63_ = 63,                       /* '~'  */
  YYSYMBOL_64_ = 64,                       /* '!'  */
  YYSYMBOL_65_ = 65,                       /* '@'  */
  YYSYMBOL_opINTNAME = 66,                 /* opINTNAME  */
  YYSYMBOL_opINTNAMER = 67,                /* opINTNAMER  */
  YYSYMBOL_opMINUSMINUS = 68,              /* opMINUSMINUS  */
  YYSYMBOL_opPLUSPLUS = 69,                /* opPLUSPLUS  */
  YYSYMBOL_STMT_SEP = 70,                  /* STMT_SEP  */
  YYSYMBOL_opSHL = 71,                     /* opSHL  */
  YYSYMBOL_opSHR = 72,                     /* opSHR  */
  YYSYMBOL_opPLASN = 73,                   /* opPLASN  */
  YYSYMBOL_opMIASN = 74,                   /* opMIASN  */
  YYSYMBOL_opMLASN = 75,                   /* opMLASN  */
  YYSYMBOL_opDVASN = 76,                   /* opDVASN  */
  YYSYMBOL_opMODASN = 77,                  /* opMODASN  */
  YYSYMBOL_opANDASN = 78,                  /* opANDASN  */
  YYSYMBOL_opXORASN = 79,                  /* opXORASN  */
  YYSYMBOL_opORASN = 80,                   /* opORASN  */
  YYSYMBOL_opSLASN = 81,                   /* opSLASN  */
  YYSYMBOL_opSRASN = 82,                   /* opSRASN  */
  YYSYMBOL_opCAT = 83,                     /* opCAT  */
  YYSYMBOL_opEQ = 84,                      /* opEQ  */
  YYSYMBOL_opNE = 85,                      /* opNE  */
  YYSYMBOL_opGE = 86,                      /* opGE  */
  YYSYMBOL_opLE = 87,                      /* opLE  */
  YYSYMBOL_opAND = 88,                     /* opAND  */
  YYSYMBOL_opOR = 89,                      /* opOR  */
  YYSYMBOL_opSTREQ = 90,                   /* opSTREQ  */
  YYSYMBOL_opCOLONCOLON = 91,              /* opCOLONCOLON  */
  YYSYMBOL_92_ = 92,                       /* '['  */
  YYSYMBOL_opMDASN = 93,                   /* opMDASN  */
  YYSYMBOL_opNDASN = 94,                   /* opNDASN  */
  YYSYMBOL_opNTASN = 95,                   /* opNTASN  */
  YYSYMBOL_96_ = 96,                       /* '?'  */
  YYSYMBOL_opSTRNE = 97,                   /* opSTRNE  */
  YYSYMBOL_UNARY = 98,                     /* UNARY  */
  YYSYMBOL_99_ = 99,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 100,                 /* $accept  */
  YYSYMBOL_start = 101,                    /* start  */
  YYSYMBOL_decl_list = 102,                /* decl_list  */
  YYSYMBOL_decl = 103,                     /* decl  */
  YYSYMBOL_package_decl = 104,             /* package_decl  */
  YYSYMBOL_fn_decl_list = 105,             /* fn_decl_list  */
  YYSYMBOL_statement_list = 106,           /* statement_list  */
  YYSYMBOL_stmt = 107,                     /* stmt  */
  YYSYMBOL_fn_decl_stmt = 108,             /* fn_decl_stmt  */
  YYSYMBOL_var_list_decl = 109,            /* var_list_decl  */
  YYSYMBOL_var_list = 110,                 /* var_list  */
  YYSYMBOL_datablock_decl = 111,           /* datablock_decl  */
  YYSYMBOL_object_decl = 112,              /* object_decl  */
  YYSYMBOL_parent_block = 113,             /* parent_block  */
  YYSYMBOL_object_name = 114,              /* object_name  */
  YYSYMBOL_object_args = 115,              /* object_args  */
  YYSYMBOL_object_declare_block = 116,     /* object_declare_block  */
  YYSYMBOL_object_decl_list = 117,         /* object_decl_list  */
  YYSYMBOL_stmt_block = 118,               /* stmt_block  */
  YYSYMBOL_switch_stmt = 119,              /* switch_stmt  */
  YYSYMBOL_case_block = 120,               /* case_block  */
  YYSYMBOL_case_expr = 121,                /* case_expr  */
  YYSYMBOL_if_stmt = 122,                  /* if_stmt  */
  YYSYMBOL_while_stmt = 123,               /* while_stmt  */
  YYSYMBOL_for_stmt = 124,                 /* for_stmt  */
  YYSYMBOL_foreach_stmt = 125,             /* foreach_stmt  */
  YYSYMBOL_expression_stmt = 126,          /* expression_stmt  */
  YYSYMBOL_expr = 127,                     /* expr  */
  YYSYMBOL_slot_acc = 128,                 /* slot_acc  */
  YYSYMBOL_intslot_acc = 129,              /* intslot_acc  */
  YYSYMBOL_class_name_expr = 130,          /* class_name_expr  */
  YYSYMBOL_assign_op_struct = 131,         /* assign_op_struct  */
  YYSYMBOL_stmt_expr = 132,                /* stmt_expr  */
  YYSYMBOL_funcall_expr = 133,             /* funcall_expr  */
  YYSYMBOL_assert_expr = 134,              /* assert_expr  */
  YYSYMBOL_expr_list_decl = 135,           /* expr_list_decl  */
  YYSYMBOL_expr_list = 136,                /* expr_list  */
  YYSYMBOL_slot_assign_list_opt = 137,     /* slot_assign_list_opt  */
  YYSYMBOL_slot_assign_list = 138,         /* slot_assign_list  */
  YYSYMBOL_slot_assign = 139,              /* slot_assign  */
  YYSYMBOL_aidx_expr = 140                 /* aidx_expr  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2845

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  100
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  41
/* YYNRULES -- Number of rules.  */
#define YYNRULES  162
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  381

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   329


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    64,     2,     2,     2,    54,    53,     2,
      55,    56,    46,    44,    57,    45,    51,    47,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    58,    59,
      48,    50,    49,    96,    65,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    92,     2,    99,    62,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    60,    52,    61,    63,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    93,    94,    95,    97,    98
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   161,   161,   167,   168,   173,   175,   177,   182,   187,
     189,   195,   196,   201,   202,   203,   204,   205,   206,   207,
     209,   211,   213,   215,   217,   219,   221,   226,   228,   234,
     235,   240,   242,   247,   252,   254,   256,   258,   260,   262,
     268,   269,   275,   276,   282,   283,   289,   290,   292,   294,
     299,   301,   306,   308,   313,   315,   320,   322,   324,   329,
     331,   336,   338,   343,   345,   350,   352,   354,   356,   358,
     360,   362,   364,   369,   371,   376,   381,   383,   385,   387,
     389,   391,   393,   395,   397,   399,   401,   403,   405,   407,
     409,   411,   413,   415,   417,   419,   421,   423,   425,   427,
     429,   431,   433,   435,   437,   439,   441,   443,   445,   447,
     449,   451,   453,   455,   457,   462,   464,   469,   471,   476,
     478,   483,   485,   487,   489,   491,   493,   495,   497,   499,
     501,   503,   505,   510,   512,   514,   516,   518,   520,   522,
     524,   526,   528,   533,   535,   537,   542,   544,   550,   551,
     556,   558,   564,   565,   570,   572,   577,   579,   581,   583,
     585,   590,   592
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "rwDEFINE", "rwENDDEF",
  "rwDECLARE", "rwDECLARESINGLETON", "rwBREAK", "rwELSE", "rwCONTINUE",
  "rwGLOBAL", "rwIF", "rwNIL", "rwRETURN", "rwWHILE", "rwDO", "rwENDIF",
  "rwENDWHILE", "rwENDFOR", "rwDEFAULT", "rwFOR", "rwFOREACH",
  "rwFOREACHSTR", "rwIN", "rwDATABLOCK", "rwSWITCH", "rwCASE",
  "rwSWITCHSTR", "rwCASEOR", "rwPACKAGE", "rwNAMESPACE", "rwCLASS",
  "rwASSERT", "ILLEGAL_TOKEN", "CHRCONST", "INTCONST", "TTAG", "VAR",
  "IDENT", "TYPEIDENT", "DOCBLOCK", "STRATOM", "TAGATOM", "FLTCONST",
  "'+'", "'-'", "'*'", "'/'", "'<'", "'>'", "'='", "'.'", "'|'", "'&'",
  "'%'", "'('", "')'", "','", "':'", "';'", "'{'", "'}'", "'^'", "'~'",
  "'!'", "'@'", "opINTNAME", "opINTNAMER", "opMINUSMINUS", "opPLUSPLUS",
  "STMT_SEP", "opSHL", "opSHR", "opPLASN", "opMIASN", "opMLASN", "opDVASN",
  "opMODASN", "opANDASN", "opXORASN", "opORASN", "opSLASN", "opSRASN",
  "opCAT", "opEQ", "opNE", "opGE", "opLE", "opAND", "opOR", "opSTREQ",
  "opCOLONCOLON", "'['", "opMDASN", "opNDASN", "opNTASN", "'?'", "opSTRNE",
  "UNARY", "']'", "$accept", "start", "decl_list", "decl", "package_decl",
  "fn_decl_list", "statement_list", "stmt", "fn_decl_stmt",
  "var_list_decl", "var_list", "datablock_decl", "object_decl",
  "parent_block", "object_name", "object_args", "object_declare_block",
  "object_decl_list", "stmt_block", "switch_stmt", "case_block",
  "case_expr", "if_stmt", "while_stmt", "for_stmt", "foreach_stmt",
  "expression_stmt", "expr", "slot_acc", "intslot_acc", "class_name_expr",
  "assign_op_struct", "stmt_expr", "funcall_expr", "assert_expr",
  "expr_list_decl", "expr_list", "slot_assign_list_opt",
  "slot_assign_list", "slot_assign", "aidx_expr", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-307)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-76)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -307,    32,   371,  -307,    -4,   -15,   -15,   -12,    -7,    15,
     300,    17,   463,    23,    26,    47,   -15,    48,    49,    62,
      58,  -307,    69,   178,   -33,  -307,  -307,  -307,  -307,  1018,
    1018,  1018,  1018,  1018,  -307,  -307,  -307,  -307,  -307,  -307,
    -307,  -307,  -307,  -307,  -307,    61,  2481,  2743,  -307,    82,
    -307,  -307,   -29,  -307,  1018,    88,    92,  -307,  -307,  1018,
    -307,  -307,  -307,  1131,  -307,  1018,  -307,  -307,   143,   682,
     121,   122,   110,  1018,  1018,   104,  1018,  1018,  1018,  -307,
    -307,  1018,  1018,  1018,  1018,  1018,  1018,  1018,  1018,  1018,
    1018,  1018,  -307,  1018,   128,   -21,   -21,  1185,   -21,   -21,
    -307,  1018,  1018,  1018,  1018,  1018,  1018,   130,  1018,  1018,
    1018,  1018,  1018,   -15,   -15,  1018,  1018,  1018,  1018,  1018,
    1018,  1018,  1018,  1018,  1018,  1018,   724,  -307,   134,   135,
    1239,   147,  1018,  1293,  -307,  1347,   523,   117,   766,  1401,
     151,   155,  1018,  1455,  1509,   183,  1023,  1077,  2481,  2481,
    2481,  2481,  2481,  2481,  2481,  2481,  2481,  2481,  2481,  2481,
     -50,  2481,   125,   139,   136,  -307,   109,   109,   -21,   -21,
    2738,  2738,   -31,  2622,  2680,   -21,  2651,   250,  -307,  -307,
     175,   175,  2709,  2709,  2738,  2738,  2593,  2564,   250,  1563,
     250,  1018,  2481,  -307,   142,   146,   144,  -307,  1018,   148,
    2481,   148,   463,   463,  -307,  -307,  1018,   808,  1617,   850,
    1018,  1018,  1671,   145,   149,     8,  -307,  -307,   166,  1018,
    -307,  1018,  2763,  -307,  1018,  1018,  1018,  1018,  1018,   -32,
     154,   171,   134,   119,   185,   167,   167,   227,  -307,  1725,
     463,  1779,   892,   934,  1833,  1887,  1941,   180,   211,   211,
     181,  -307,   187,  1995,  2481,  1018,  -307,  2481,   188,   189,
     -44,  2535,  -307,  -307,  -307,   194,   148,  -307,  1018,   206,
     207,   463,  -307,  -307,   463,   463,  2049,   463,  2103,   976,
     463,   463,   148,  1018,   205,   210,  -307,  -307,  -307,  2481,
    -307,  -307,  -307,   568,   208,   167,   139,   216,   217,  -307,
    -307,  -307,   463,  -307,   463,   463,  2157,  -307,  -307,   222,
     -10,  2481,  -307,  -307,  -307,  -307,   228,   156,   156,  -307,
    -307,  -307,   463,     3,  1018,  -307,   613,   223,   236,   -38,
     229,   234,   226,    45,   156,  -307,   238,  -307,   247,     3,
    2481,   418,  -307,   156,  1018,  1018,  1018,   -35,  -307,  -307,
     251,    45,  -307,  -307,   252,   230,  -307,   248,  2211,  2265,
     -40,  1018,  1018,  -307,  -307,  -307,  -307,  -307,  -307,   262,
    2319,   -36,   658,  1018,  -307,   263,  2373,  1018,  -307,  2427,
    -307
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     2,     1,     0,     0,     0,   108,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,    88,   113,   111,    26,   112,   105,   106,     0,
       0,     0,     0,     0,     4,     7,     5,     6,    17,   135,
      18,    13,    14,    15,    16,     0,     0,   109,   110,    76,
     133,   134,     0,   119,     0,     0,     0,    19,    20,     0,
     108,    88,    21,     0,    76,     0,    11,    53,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   122,
     121,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   138,   148,     0,    86,    87,     0,   104,   103,
      23,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   140,    29,     0,
       0,    42,    42,     0,    22,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   136,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   161,
       0,   150,     0,   149,     0,    77,    82,    83,    84,    85,
      90,    91,   115,    81,    80,    79,    78,   102,   117,   118,
      97,    98,    94,    95,    92,    93,    99,    96,   100,     0,
     101,     0,   141,    31,     0,    30,     0,   120,    42,    40,
      43,    40,     0,     0,    52,    12,     0,     0,     0,     0,
       0,     0,    40,     0,     0,     0,     9,   146,     0,     0,
      24,     0,   114,   143,     0,   148,   148,     0,     0,     0,
       0,     0,    29,     0,     0,    44,    44,    61,    63,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    10,     0,     0,   162,     0,   139,   151,     0,     0,
       0,    89,   142,    11,    32,     0,    40,    41,     0,     0,
       0,     0,    64,    72,     0,     0,     0,     0,     0,     0,
       0,     0,    40,     0,     0,     0,     8,   147,    25,   137,
     144,   145,   116,     0,     0,    44,    45,    35,    39,    62,
      71,    70,     0,    68,     0,     0,     0,    73,    74,     0,
       0,    59,    54,    55,    27,    11,     0,    46,    46,    69,
      67,    66,     0,   152,     0,    11,     0,    37,     0,     0,
       0,     0,     0,    48,    47,   154,     0,    65,     0,   153,
      60,    56,    28,    46,     0,     0,     0,     0,    50,    34,
       0,    49,   155,    38,     0,     0,    58,     0,     0,     0,
       0,     0,     0,    51,    33,    11,    36,   158,   156,     0,
       0,     0,    57,     0,   157,     0,     0,     0,   159,     0,
     160
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -307,  -307,  -307,  -307,  -307,  -307,  -259,    -1,  -142,    83,
    -307,  -307,   -53,  -181,  -124,  -231,  -299,   -20,  -135,  -307,
    -243,  -307,  -307,  -307,  -307,  -307,  -307,     6,  -307,  -307,
      37,   -45,    -2,  -307,  -307,  -149,  -182,  -307,    -5,  -306,
    -213
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,     2,    34,    35,   215,   136,    67,    37,   194,
     195,    38,    39,   235,   199,   269,   332,   333,    68,    40,
     284,   310,    41,    42,    43,    44,    45,    46,    47,    48,
      55,    92,    64,    50,    51,   162,   163,   338,   334,   335,
     160
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      49,    36,   127,   216,   293,   270,   285,   221,   201,   229,
      49,     4,   345,   221,   260,   361,    63,   221,   324,   336,
     236,   221,    93,    53,   226,   224,   128,   328,   352,   262,
     107,   247,     3,   352,    52,    95,    96,    97,    98,    99,
      54,   329,   330,    56,   357,   113,   114,    57,   325,   222,
       5,     6,    58,    72,   346,   292,   326,   362,    94,   369,
     130,   227,   129,   375,   316,   133,   341,   237,   238,   250,
      59,   135,    65,   251,   233,   139,   258,   259,    69,   143,
     144,    70,   146,   147,   148,   295,   296,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   356,   161,
      75,   309,    71,    73,    74,   273,   372,   166,   167,   168,
     169,   170,   171,    76,   173,   174,   175,   176,   177,    77,
     100,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   192,   360,    49,   205,   299,   200,   200,   300,
     301,   -75,   303,   131,   208,   307,   308,   132,   212,   371,
     178,   179,     5,     6,    60,   103,   104,   137,   140,   141,
     107,     5,     6,   110,   145,   142,   164,   319,   172,   320,
     321,   193,   206,   196,   210,   113,   114,   256,   211,    20,
     328,   223,    21,    61,    23,    24,     4,   337,    26,    27,
      28,   225,    29,    30,   329,   330,   224,   161,   230,   232,
      49,    49,    31,   231,   200,   248,   234,   252,   264,   249,
      32,    33,   239,   241,   263,   244,   245,   246,   266,   101,
     102,   103,   104,   267,   268,   253,   107,   254,    78,   110,
     257,   161,   161,   159,   261,   271,   282,   283,    49,   198,
     286,   113,   114,   287,   290,   291,    79,    80,   276,   278,
     294,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,   289,   297,   298,   331,   331,   312,   347,   315,    49,
      91,   313,    49,    49,   161,    49,   317,   318,    49,    49,
     350,   331,   323,   343,   327,   306,   344,   349,   365,   311,
     331,    49,   205,   348,   101,   102,   103,   104,   350,   353,
      49,   107,    49,    49,   110,     5,     6,    60,   354,   366,
     363,   364,   373,   377,   351,   265,   113,   114,   339,     0,
      49,   115,   116,     0,    49,   205,     0,     0,     0,     0,
     340,     0,    20,     0,     0,    21,    61,    23,    24,    49,
     205,    26,    27,    28,     0,    29,    30,     0,     0,     0,
     358,   359,   159,     0,     0,    31,     0,     0,     0,    62,
       0,     0,     0,    32,    33,     0,     0,   370,   159,     0,
      49,   205,     0,     0,     4,     0,     5,     6,     7,   376,
       8,     0,     9,   379,    10,    11,    12,     0,     0,     0,
       0,    13,    14,    15,     0,    16,    17,     0,    18,     0,
      19,     0,     0,    20,     0,     0,    21,    22,    23,    24,
       0,    25,    26,    27,    28,     0,    29,    30,     0,     0,
       0,     0,     0,     5,     6,     7,    31,     8,     0,     9,
       0,    10,    11,    12,    32,    33,     0,   355,    13,    14,
      15,     0,    16,    17,   283,    18,     0,     0,     0,     0,
      20,     0,     0,    21,    22,    23,    24,     0,    25,    26,
      27,    28,     0,    29,    30,     0,     0,     0,     5,     6,
       7,     0,     8,    31,     9,     0,    10,    11,    12,     0,
       0,    32,    33,    13,    14,    15,     0,    16,    17,     0,
      18,     0,     0,     0,     0,    20,     0,     0,    21,    22,
      23,    24,     0,    25,    26,    27,    28,     0,    29,    30,
       0,     0,     0,     0,     0,     0,     0,     0,    31,     0,
       0,     0,     0,    66,     0,     0,    32,    33,     5,     6,
       7,     0,     8,     0,     9,     0,    10,    11,    12,     0,
       0,     0,     0,    13,    14,    15,     0,    16,    17,     0,
      18,     0,     0,     0,     0,    20,     0,     0,    21,    22,
      23,    24,     0,    25,    26,    27,    28,     0,    29,    30,
       0,     0,     0,     5,     6,     7,     0,     8,    31,     9,
       0,    10,    11,    12,   204,     0,    32,    33,    13,    14,
      15,     0,    16,    17,     0,    18,     0,     0,     0,     0,
      20,     0,     0,    21,    22,    23,    24,     0,    25,    26,
      27,    28,     0,    29,    30,     0,     0,     0,     5,     6,
       7,     0,     8,    31,     9,     0,    10,    11,    12,   314,
       0,    32,    33,    13,    14,    15,     0,    16,    17,     0,
      18,     0,     0,     0,     0,    20,     0,     0,    21,    22,
      23,    24,     0,    25,    26,    27,    28,     0,    29,    30,
       0,     0,     0,     5,     6,     7,     0,     8,    31,     9,
       0,    10,    11,    12,   342,     0,    32,    33,    13,    14,
      15,     0,    16,    17,     0,    18,     0,     5,     6,    60,
      20,     0,     0,    21,    22,    23,    24,     0,    25,    26,
      27,    28,     0,    29,    30,     0,     0,     0,     0,     0,
       0,     0,     0,    31,    20,     0,     0,    21,    61,    23,
      24,    32,    33,    26,    27,    28,     0,    29,    30,     5,
       6,    60,     0,     0,     0,     0,     0,    31,     0,     0,
       0,   138,     0,     0,     0,    32,    33,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    20,     0,     0,    21,
      61,    23,    24,     0,     0,    26,    27,    28,     0,    29,
      30,     5,     6,    60,     0,     0,     0,     0,     0,    31,
       0,     0,     0,     0,   191,     0,     0,    32,    33,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    20,     0,
       0,    21,    61,    23,    24,     0,     0,    26,    27,    28,
       0,    29,    30,     5,     6,    60,     0,     0,     0,     0,
       0,    31,     0,     0,     0,   207,     0,     0,     0,    32,
      33,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      20,     0,     0,    21,    61,    23,    24,     0,     0,    26,
      27,    28,     0,    29,    30,     5,     6,    60,     0,     0,
       0,     0,     0,    31,   240,     0,     0,     0,     0,     0,
       0,    32,    33,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    20,     0,     0,    21,    61,    23,    24,     0,
       0,    26,    27,    28,     0,    29,    30,     5,     6,    60,
       0,     0,     0,     0,     0,    31,     0,     0,     0,   243,
       0,     0,     0,    32,    33,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    20,     0,     0,    21,    61,    23,
      24,     0,     0,    26,    27,    28,     0,    29,    30,     5,
       6,    60,     0,     0,     0,     0,     0,    31,   275,     0,
       0,     0,     0,     0,     0,    32,    33,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    20,     0,     0,    21,
      61,    23,    24,     0,     0,    26,    27,    28,     0,    29,
      30,     5,     6,    60,     0,     0,     0,     0,     0,    31,
     277,     0,     0,     0,     0,     0,     0,    32,    33,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    20,     0,
       0,    21,    61,    23,    24,     0,     0,    26,    27,    28,
       0,    29,    30,     5,     6,    60,     0,     0,     0,     0,
       0,    31,   305,     0,     0,     0,     0,     0,     0,    32,
      33,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      20,     0,     0,    21,    61,    23,    24,     0,     0,    26,
      27,    28,     0,    29,    30,     0,     0,   101,   102,   103,
     104,   105,   106,    31,   107,   108,   109,   110,     0,   217,
     218,    32,    33,     0,     0,   111,     0,     0,   112,   113,
     114,     0,     0,     0,   115,   116,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   117,   118,   119,
     120,   121,   122,   123,     0,     0,     0,     0,     0,   124,
     125,   101,   102,   103,   104,   105,   106,     0,   107,   108,
     109,   110,     0,     0,   219,     0,   220,     0,     0,   111,
       0,     0,   112,   113,   114,     0,     0,     0,   115,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   117,   118,   119,   120,   121,   122,   123,     0,     0,
       0,     0,     0,   124,   125,   101,   102,   103,   104,   105,
     106,     0,   107,   108,   109,   110,     0,     0,     0,     0,
     134,     0,     0,   111,     0,     0,   112,   113,   114,     0,
       0,     0,   115,   116,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   117,   118,   119,   120,   121,
     122,   123,     0,     0,     0,     0,     0,   124,   125,   101,
     102,   103,   104,   105,   106,     0,   107,   108,   109,   110,
       0,   165,     0,     0,     0,     0,     0,   111,     0,     0,
     112,   113,   114,     0,     0,     0,   115,   116,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,   123,     0,     0,     0,     0,
       0,   124,   125,   101,   102,   103,   104,   105,   106,     0,
     107,   108,   109,   110,     0,   197,     0,     0,     0,     0,
       0,   111,     0,     0,   112,   113,   114,     0,     0,     0,
     115,   116,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   117,   118,   119,   120,   121,   122,   123,
       0,     0,     0,     0,     0,   124,   125,   101,   102,   103,
     104,   105,   106,     0,   107,   108,   109,   110,     0,   202,
       0,     0,     0,     0,     0,   111,     0,     0,   112,   113,
     114,     0,     0,     0,   115,   116,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   117,   118,   119,
     120,   121,   122,   123,     0,     0,     0,     0,     0,   124,
     125,   101,   102,   103,   104,   105,   106,     0,   107,   108,
     109,   110,     0,   203,     0,     0,     0,     0,     0,   111,
       0,     0,   112,   113,   114,     0,     0,     0,   115,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   117,   118,   119,   120,   121,   122,   123,     0,     0,
       0,     0,     0,   124,   125,   101,   102,   103,   104,   105,
     106,     0,   107,   108,   109,   110,     0,     0,     0,     0,
     209,     0,     0,   111,     0,     0,   112,   113,   114,     0,
       0,     0,   115,   116,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   117,   118,   119,   120,   121,
     122,   123,     0,     0,     0,     0,     0,   124,   125,   101,
     102,   103,   104,   105,   106,     0,   107,   108,   109,   110,
       0,   213,     0,     0,     0,     0,     0,   111,     0,     0,
     112,   113,   114,     0,     0,     0,   115,   116,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,   123,     0,     0,     0,     0,
       0,   124,   125,   101,   102,   103,   104,   105,   106,     0,
     107,   108,   109,   110,     0,   214,     0,     0,     0,     0,
       0,   111,     0,     0,   112,   113,   114,     0,     0,     0,
     115,   116,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   117,   118,   119,   120,   121,   122,   123,
       0,     0,     0,     0,     0,   124,   125,   101,   102,   103,
     104,   105,   106,     0,   107,   108,   109,   110,     0,     0,
       0,   228,     0,     0,     0,   111,     0,     0,   112,   113,
     114,     0,     0,     0,   115,   116,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   117,   118,   119,
     120,   121,   122,   123,     0,     0,     0,     0,     0,   124,
     125,   101,   102,   103,   104,   105,   106,     0,   107,   108,
     109,   110,     0,     0,     0,     0,   242,     0,     0,   111,
       0,     0,   112,   113,   114,     0,     0,     0,   115,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   117,   118,   119,   120,   121,   122,   123,     0,     0,
       0,     0,     0,   124,   125,   101,   102,   103,   104,   105,
     106,     0,   107,   108,   109,   110,     0,     0,     0,   234,
       0,     0,     0,   111,     0,     0,   112,   113,   114,     0,
       0,     0,   115,   116,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   117,   118,   119,   120,   121,
     122,   123,     0,     0,     0,     0,     0,   124,   125,   101,
     102,   103,   104,   105,   106,     0,   107,   108,   109,   110,
       0,   272,     0,     0,     0,     0,     0,   111,     0,     0,
     112,   113,   114,     0,     0,     0,   115,   116,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,   123,     0,     0,     0,     0,
       0,   124,   125,   101,   102,   103,   104,   105,   106,     0,
     107,   108,   109,   110,     0,   274,     0,     0,     0,     0,
       0,   111,     0,     0,   112,   113,   114,     0,     0,     0,
     115,   116,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   117,   118,   119,   120,   121,   122,   123,
       0,     0,     0,     0,     0,   124,   125,   101,   102,   103,
     104,   105,   106,     0,   107,   108,   109,   110,     0,     0,
       0,     0,   279,     0,     0,   111,     0,     0,   112,   113,
     114,     0,     0,     0,   115,   116,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   117,   118,   119,
     120,   121,   122,   123,     0,     0,     0,     0,     0,   124,
     125,   101,   102,   103,   104,   105,   106,     0,   107,   108,
     109,   110,     0,   280,     0,     0,     0,     0,     0,   111,
       0,     0,   112,   113,   114,     0,     0,     0,   115,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   117,   118,   119,   120,   121,   122,   123,     0,     0,
       0,     0,     0,   124,   125,   101,   102,   103,   104,   105,
     106,     0,   107,   108,   109,   110,     0,   281,     0,     0,
       0,     0,     0,   111,     0,     0,   112,   113,   114,     0,
       0,     0,   115,   116,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   117,   118,   119,   120,   121,
     122,   123,     0,     0,     0,     0,     0,   124,   125,   101,
     102,   103,   104,   105,   106,     0,   107,   108,   109,   110,
       0,     0,     0,     0,   288,     0,     0,   111,     0,     0,
     112,   113,   114,     0,     0,     0,   115,   116,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,   123,     0,     0,     0,     0,
       0,   124,   125,   101,   102,   103,   104,   105,   106,     0,
     107,   108,   109,   110,     0,   302,     0,     0,     0,     0,
       0,   111,     0,     0,   112,   113,   114,     0,     0,     0,
     115,   116,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   117,   118,   119,   120,   121,   122,   123,
       0,     0,     0,     0,     0,   124,   125,   101,   102,   103,
     104,   105,   106,     0,   107,   108,   109,   110,     0,   304,
       0,     0,     0,     0,     0,   111,     0,     0,   112,   113,
     114,     0,     0,     0,   115,   116,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   117,   118,   119,
     120,   121,   122,   123,     0,     0,     0,     0,     0,   124,
     125,   101,   102,   103,   104,   105,   106,     0,   107,   108,
     109,   110,     0,   322,     0,     0,     0,     0,     0,   111,
       0,     0,   112,   113,   114,     0,     0,     0,   115,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   117,   118,   119,   120,   121,   122,   123,     0,     0,
       0,     0,     0,   124,   125,   101,   102,   103,   104,   105,
     106,     0,   107,   108,   109,   110,     0,     0,     0,     0,
     367,     0,     0,   111,     0,     0,   112,   113,   114,     0,
       0,     0,   115,   116,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   117,   118,   119,   120,   121,
     122,   123,     0,     0,     0,     0,     0,   124,   125,   101,
     102,   103,   104,   105,   106,     0,   107,   108,   109,   110,
       0,     0,     0,     0,   368,     0,     0,   111,     0,     0,
     112,   113,   114,     0,     0,     0,   115,   116,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,   123,     0,     0,     0,     0,
       0,   124,   125,   101,   102,   103,   104,   105,   106,     0,
     107,   108,   109,   110,     0,     0,     0,     0,   374,     0,
       0,   111,     0,     0,   112,   113,   114,     0,     0,     0,
     115,   116,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   117,   118,   119,   120,   121,   122,   123,
       0,     0,     0,     0,     0,   124,   125,   101,   102,   103,
     104,   105,   106,     0,   107,   108,   109,   110,     0,     0,
       0,     0,   378,     0,     0,   111,     0,     0,   112,   113,
     114,     0,     0,     0,   115,   116,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   117,   118,   119,
     120,   121,   122,   123,     0,     0,     0,     0,     0,   124,
     125,   101,   102,   103,   104,   105,   106,     0,   107,   108,
     109,   110,     0,     0,     0,     0,   380,     0,     0,   111,
       0,     0,   112,   113,   114,     0,     0,     0,   115,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   117,   118,   119,   120,   121,   122,   123,     0,     0,
       0,     0,     0,   124,   125,   101,   102,   103,   104,   105,
     106,     0,   107,   108,   109,   110,     0,     0,     0,     0,
       0,     0,     0,   111,     0,     0,   112,   113,   114,     0,
       0,     0,   115,   116,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   117,   118,   119,   120,   121,
     122,   123,     0,     0,     0,     0,     0,   124,   125,   101,
     102,   103,   104,   105,   106,     0,   107,   108,   109,   110,
       0,     0,     0,     0,     0,     0,     0,   111,     0,     0,
     112,   113,   114,     0,     0,     0,   115,   116,   101,   102,
     103,   104,   105,   106,     0,   107,   108,   109,   110,   117,
     118,   119,   120,   121,   122,   123,   111,     0,     0,   112,
     113,   114,   125,     0,     0,   115,   116,   101,   102,   103,
     104,   105,   106,     0,   107,   108,   109,   110,   117,   118,
     119,   120,   121,     0,   123,   111,     0,     0,   112,   113,
     114,   125,     0,     0,   115,   116,   101,   102,   103,   104,
     105,   106,     0,   107,     0,   109,   110,   117,   118,   119,
     120,     0,     0,   123,   111,     0,     0,   112,   113,   114,
     125,     0,     0,   115,   116,   101,   102,   103,   104,   105,
     106,     0,   107,     0,   109,   110,   117,   118,   119,   120,
       0,     0,   123,     0,     0,     0,   112,   113,   114,   125,
       0,     0,   115,   116,   101,   102,   103,   104,   105,   106,
       0,   107,     0,     0,   110,   117,   118,   119,   120,     0,
       0,   123,     0,     0,     0,   112,   113,   114,   125,     0,
       0,   115,   116,   101,   102,   103,   104,   105,   106,     0,
     107,     0,     0,   110,   117,   118,   119,   120,     0,     0,
     123,     0,     0,     0,   112,   113,   114,   125,     0,     0,
     115,   116,   101,   102,   103,   104,     0,     0,     0,   107,
       0,     0,   110,   126,     0,   119,   120,     0,     0,   123,
       0,     0,     0,   112,   113,   114,   125,     0,     0,   115,
     116,    79,    80,   255,     0,     0,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,     0,     0,   123,     0,
       0,    79,    80,     0,     0,   125,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90
};

static const yytype_int16 yycheck[] =
{
       2,     2,    47,   145,   263,   236,   249,    57,   132,   191,
      12,     3,    50,    57,   227,    50,    10,    57,    28,   318,
     201,    57,    55,    38,    55,    57,    55,    24,   334,    61,
      51,   212,     0,   339,    38,    29,    30,    31,    32,    33,
      55,    38,    39,     6,   343,    66,    67,    59,    58,    99,
       5,     6,    59,    16,    92,    99,   315,    92,    91,    99,
      54,    92,    91,    99,   295,    59,   325,   202,   203,    61,
      55,    65,    55,   215,   198,    69,   225,   226,    55,    73,
      74,    55,    76,    77,    78,   266,   268,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,   341,    93,
      38,   282,    55,    55,    55,   240,   365,   101,   102,   103,
     104,   105,   106,    55,   108,   109,   110,   111,   112,    50,
      59,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   346,   136,   136,   271,   131,   132,   274,
     275,    59,   277,    55,   138,   280,   281,    55,   142,   362,
     113,   114,     5,     6,     7,    46,    47,    14,    37,    37,
      51,     5,     6,    54,    60,    55,    38,   302,    38,   304,
     305,    37,    55,    38,    23,    66,    67,   222,    23,    32,
      24,    56,    35,    36,    37,    38,     3,   322,    41,    42,
      43,    55,    45,    46,    38,    39,    57,   191,    56,    55,
     202,   203,    55,    57,   198,    60,    58,    41,    37,    60,
      63,    64,   206,   207,    60,   209,   210,   211,    99,    44,
      45,    46,    47,    38,    57,   219,    51,   221,    50,    54,
     224,   225,   226,   227,   228,     8,    56,    26,   240,    92,
      59,    66,    67,    56,    56,    56,    68,    69,   242,   243,
      56,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,   255,    56,    56,   317,   318,    61,    38,    60,   271,
      92,    61,   274,   275,   268,   277,    60,    60,   280,   281,
     333,   334,    60,    60,    56,   279,    50,    61,    58,   283,
     343,   293,   293,    59,    44,    45,    46,    47,   351,    61,
     302,    51,   304,   305,    54,     5,     6,     7,    61,    61,
      59,    59,    50,    50,   334,   232,    66,    67,   323,    -1,
     322,    71,    72,    -1,   326,   326,    -1,    -1,    -1,    -1,
     324,    -1,    32,    -1,    -1,    35,    36,    37,    38,   341,
     341,    41,    42,    43,    -1,    45,    46,    -1,    -1,    -1,
     344,   345,   346,    -1,    -1,    55,    -1,    -1,    -1,    59,
      -1,    -1,    -1,    63,    64,    -1,    -1,   361,   362,    -1,
     372,   372,    -1,    -1,     3,    -1,     5,     6,     7,   373,
       9,    -1,    11,   377,    13,    14,    15,    -1,    -1,    -1,
      -1,    20,    21,    22,    -1,    24,    25,    -1,    27,    -1,
      29,    -1,    -1,    32,    -1,    -1,    35,    36,    37,    38,
      -1,    40,    41,    42,    43,    -1,    45,    46,    -1,    -1,
      -1,    -1,    -1,     5,     6,     7,    55,     9,    -1,    11,
      -1,    13,    14,    15,    63,    64,    -1,    19,    20,    21,
      22,    -1,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    35,    36,    37,    38,    -1,    40,    41,
      42,    43,    -1,    45,    46,    -1,    -1,    -1,     5,     6,
       7,    -1,     9,    55,    11,    -1,    13,    14,    15,    -1,
      -1,    63,    64,    20,    21,    22,    -1,    24,    25,    -1,
      27,    -1,    -1,    -1,    -1,    32,    -1,    -1,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    -1,    45,    46,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,    -1,
      -1,    -1,    -1,    60,    -1,    -1,    63,    64,     5,     6,
       7,    -1,     9,    -1,    11,    -1,    13,    14,    15,    -1,
      -1,    -1,    -1,    20,    21,    22,    -1,    24,    25,    -1,
      27,    -1,    -1,    -1,    -1,    32,    -1,    -1,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    -1,    45,    46,
      -1,    -1,    -1,     5,     6,     7,    -1,     9,    55,    11,
      -1,    13,    14,    15,    61,    -1,    63,    64,    20,    21,
      22,    -1,    24,    25,    -1,    27,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    35,    36,    37,    38,    -1,    40,    41,
      42,    43,    -1,    45,    46,    -1,    -1,    -1,     5,     6,
       7,    -1,     9,    55,    11,    -1,    13,    14,    15,    61,
      -1,    63,    64,    20,    21,    22,    -1,    24,    25,    -1,
      27,    -1,    -1,    -1,    -1,    32,    -1,    -1,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    -1,    45,    46,
      -1,    -1,    -1,     5,     6,     7,    -1,     9,    55,    11,
      -1,    13,    14,    15,    61,    -1,    63,    64,    20,    21,
      22,    -1,    24,    25,    -1,    27,    -1,     5,     6,     7,
      32,    -1,    -1,    35,    36,    37,    38,    -1,    40,    41,
      42,    43,    -1,    45,    46,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    55,    32,    -1,    -1,    35,    36,    37,
      38,    63,    64,    41,    42,    43,    -1,    45,    46,     5,
       6,     7,    -1,    -1,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    59,    -1,    -1,    -1,    63,    64,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    32,    -1,    -1,    35,
      36,    37,    38,    -1,    -1,    41,    42,    43,    -1,    45,
      46,     5,     6,     7,    -1,    -1,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    60,    -1,    -1,    63,    64,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,    -1,
      -1,    35,    36,    37,    38,    -1,    -1,    41,    42,    43,
      -1,    45,    46,     5,     6,     7,    -1,    -1,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    59,    -1,    -1,    -1,    63,
      64,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    35,    36,    37,    38,    -1,    -1,    41,
      42,    43,    -1,    45,    46,     5,     6,     7,    -1,    -1,
      -1,    -1,    -1,    55,    56,    -1,    -1,    -1,    -1,    -1,
      -1,    63,    64,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    32,    -1,    -1,    35,    36,    37,    38,    -1,
      -1,    41,    42,    43,    -1,    45,    46,     5,     6,     7,
      -1,    -1,    -1,    -1,    -1,    55,    -1,    -1,    -1,    59,
      -1,    -1,    -1,    63,    64,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    32,    -1,    -1,    35,    36,    37,
      38,    -1,    -1,    41,    42,    43,    -1,    45,    46,     5,
       6,     7,    -1,    -1,    -1,    -1,    -1,    55,    56,    -1,
      -1,    -1,    -1,    -1,    -1,    63,    64,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    32,    -1,    -1,    35,
      36,    37,    38,    -1,    -1,    41,    42,    43,    -1,    45,
      46,     5,     6,     7,    -1,    -1,    -1,    -1,    -1,    55,
      56,    -1,    -1,    -1,    -1,    -1,    -1,    63,    64,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,    -1,
      -1,    35,    36,    37,    38,    -1,    -1,    41,    42,    43,
      -1,    45,    46,     5,     6,     7,    -1,    -1,    -1,    -1,
      -1,    55,    56,    -1,    -1,    -1,    -1,    -1,    -1,    63,
      64,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      32,    -1,    -1,    35,    36,    37,    38,    -1,    -1,    41,
      42,    43,    -1,    45,    46,    -1,    -1,    44,    45,    46,
      47,    48,    49,    55,    51,    52,    53,    54,    -1,    56,
      57,    63,    64,    -1,    -1,    62,    -1,    -1,    65,    66,
      67,    -1,    -1,    -1,    71,    72,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,    85,    86,
      87,    88,    89,    90,    -1,    -1,    -1,    -1,    -1,    96,
      97,    44,    45,    46,    47,    48,    49,    -1,    51,    52,
      53,    54,    -1,    -1,    57,    -1,    59,    -1,    -1,    62,
      -1,    -1,    65,    66,    67,    -1,    -1,    -1,    71,    72,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    84,    85,    86,    87,    88,    89,    90,    -1,    -1,
      -1,    -1,    -1,    96,    97,    44,    45,    46,    47,    48,
      49,    -1,    51,    52,    53,    54,    -1,    -1,    -1,    -1,
      59,    -1,    -1,    62,    -1,    -1,    65,    66,    67,    -1,
      -1,    -1,    71,    72,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    84,    85,    86,    87,    88,
      89,    90,    -1,    -1,    -1,    -1,    -1,    96,    97,    44,
      45,    46,    47,    48,    49,    -1,    51,    52,    53,    54,
      -1,    56,    -1,    -1,    -1,    -1,    -1,    62,    -1,    -1,
      65,    66,    67,    -1,    -1,    -1,    71,    72,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,
      85,    86,    87,    88,    89,    90,    -1,    -1,    -1,    -1,
      -1,    96,    97,    44,    45,    46,    47,    48,    49,    -1,
      51,    52,    53,    54,    -1,    56,    -1,    -1,    -1,    -1,
      -1,    62,    -1,    -1,    65,    66,    67,    -1,    -1,    -1,
      71,    72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    84,    85,    86,    87,    88,    89,    90,
      -1,    -1,    -1,    -1,    -1,    96,    97,    44,    45,    46,
      47,    48,    49,    -1,    51,    52,    53,    54,    -1,    56,
      -1,    -1,    -1,    -1,    -1,    62,    -1,    -1,    65,    66,
      67,    -1,    -1,    -1,    71,    72,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,    85,    86,
      87,    88,    89,    90,    -1,    -1,    -1,    -1,    -1,    96,
      97,    44,    45,    46,    47,    48,    49,    -1,    51,    52,
      53,    54,    -1,    56,    -1,    -1,    -1,    -1,    -1,    62,
      -1,    -1,    65,    66,    67,    -1,    -1,    -1,    71,    72,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    84,    85,    86,    87,    88,    89,    90,    -1,    -1,
      -1,    -1,    -1,    96,    97,    44,    45,    46,    47,    48,
      49,    -1,    51,    52,    53,    54,    -1,    -1,    -1,    -1,
      59,    -1,    -1,    62,    -1,    -1,    65,    66,    67,    -1,
      -1,    -1,    71,    72,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    84,    85,    86,    87,    88,
      89,    90,    -1,    -1,    -1,    -1,    -1,    96,    97,    44,
      45,    46,    47,    48,    49,    -1,    51,    52,    53,    54,
      -1,    56,    -1,    -1,    -1,    -1,    -1,    62,    -1,    -1,
      65,    66,    67,    -1,    -1,    -1,    71,    72,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,
      85,    86,    87,    88,    89,    90,    -1,    -1,    -1,    -1,
      -1,    96,    97,    44,    45,    46,    47,    48,    49,    -1,
      51,    52,    53,    54,    -1,    56,    -1,    -1,    -1,    -1,
      -1,    62,    -1,    -1,    65,    66,    67,    -1,    -1,    -1,
      71,    72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    84,    85,    86,    87,    88,    89,    90,
      -1,    -1,    -1,    -1,    -1,    96,    97,    44,    45,    46,
      47,    48,    49,    -1,    51,    52,    53,    54,    -1,    -1,
      -1,    58,    -1,    -1,    -1,    62,    -1,    -1,    65,    66,
      67,    -1,    -1,    -1,    71,    72,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,    85,    86,
      87,    88,    89,    90,    -1,    -1,    -1,    -1,    -1,    96,
      97,    44,    45,    46,    47,    48,    49,    -1,    51,    52,
      53,    54,    -1,    -1,    -1,    -1,    59,    -1,    -1,    62,
      -1,    -1,    65,    66,    67,    -1,    -1,    -1,    71,    72,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    84,    85,    86,    87,    88,    89,    90,    -1,    -1,
      -1,    -1,    -1,    96,    97,    44,    45,    46,    47,    48,
      49,    -1,    51,    52,    53,    54,    -1,    -1,    -1,    58,
      -1,    -1,    -1,    62,    -1,    -1,    65,    66,    67,    -1,
      -1,    -1,    71,    72,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    84,    85,    86,    87,    88,
      89,    90,    -1,    -1,    -1,    -1,    -1,    96,    97,    44,
      45,    46,    47,    48,    49,    -1,    51,    52,    53,    54,
      -1,    56,    -1,    -1,    -1,    -1,    -1,    62,    -1,    -1,
      65,    66,    67,    -1,    -1,    -1,    71,    72,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,
      85,    86,    87,    88,    89,    90,    -1,    -1,    -1,    -1,
      -1,    96,    97,    44,    45,    46,    47,    48,    49,    -1,
      51,    52,    53,    54,    -1,    56,    -1,    -1,    -1,    -1,
      -1,    62,    -1,    -1,    65,    66,    67,    -1,    -1,    -1,
      71,    72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    84,    85,    86,    87,    88,    89,    90,
      -1,    -1,    -1,    -1,    -1,    96,    97,    44,    45,    46,
      47,    48,    49,    -1,    51,    52,    53,    54,    -1,    -1,
      -1,    -1,    59,    -1,    -1,    62,    -1,    -1,    65,    66,
      67,    -1,    -1,    -1,    71,    72,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,    85,    86,
      87,    88,    89,    90,    -1,    -1,    -1,    -1,    -1,    96,
      97,    44,    45,    46,    47,    48,    49,    -1,    51,    52,
      53,    54,    -1,    56,    -1,    -1,    -1,    -1,    -1,    62,
      -1,    -1,    65,    66,    67,    -1,    -1,    -1,    71,    72,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    84,    85,    86,    87,    88,    89,    90,    -1,    -1,
      -1,    -1,    -1,    96,    97,    44,    45,    46,    47,    48,
      49,    -1,    51,    52,    53,    54,    -1,    56,    -1,    -1,
      -1,    -1,    -1,    62,    -1,    -1,    65,    66,    67,    -1,
      -1,    -1,    71,    72,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    84,    85,    86,    87,    88,
      89,    90,    -1,    -1,    -1,    -1,    -1,    96,    97,    44,
      45,    46,    47,    48,    49,    -1,    51,    52,    53,    54,
      -1,    -1,    -1,    -1,    59,    -1,    -1,    62,    -1,    -1,
      65,    66,    67,    -1,    -1,    -1,    71,    72,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,
      85,    86,    87,    88,    89,    90,    -1,    -1,    -1,    -1,
      -1,    96,    97,    44,    45,    46,    47,    48,    49,    -1,
      51,    52,    53,    54,    -1,    56,    -1,    -1,    -1,    -1,
      -1,    62,    -1,    -1,    65,    66,    67,    -1,    -1,    -1,
      71,    72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    84,    85,    86,    87,    88,    89,    90,
      -1,    -1,    -1,    -1,    -1,    96,    97,    44,    45,    46,
      47,    48,    49,    -1,    51,    52,    53,    54,    -1,    56,
      -1,    -1,    -1,    -1,    -1,    62,    -1,    -1,    65,    66,
      67,    -1,    -1,    -1,    71,    72,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,    85,    86,
      87,    88,    89,    90,    -1,    -1,    -1,    -1,    -1,    96,
      97,    44,    45,    46,    47,    48,    49,    -1,    51,    52,
      53,    54,    -1,    56,    -1,    -1,    -1,    -1,    -1,    62,
      -1,    -1,    65,    66,    67,    -1,    -1,    -1,    71,    72,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    84,    85,    86,    87,    88,    89,    90,    -1,    -1,
      -1,    -1,    -1,    96,    97,    44,    45,    46,    47,    48,
      49,    -1,    51,    52,    53,    54,    -1,    -1,    -1,    -1,
      59,    -1,    -1,    62,    -1,    -1,    65,    66,    67,    -1,
      -1,    -1,    71,    72,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    84,    85,    86,    87,    88,
      89,    90,    -1,    -1,    -1,    -1,    -1,    96,    97,    44,
      45,    46,    47,    48,    49,    -1,    51,    52,    53,    54,
      -1,    -1,    -1,    -1,    59,    -1,    -1,    62,    -1,    -1,
      65,    66,    67,    -1,    -1,    -1,    71,    72,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,
      85,    86,    87,    88,    89,    90,    -1,    -1,    -1,    -1,
      -1,    96,    97,    44,    45,    46,    47,    48,    49,    -1,
      51,    52,    53,    54,    -1,    -1,    -1,    -1,    59,    -1,
      -1,    62,    -1,    -1,    65,    66,    67,    -1,    -1,    -1,
      71,    72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    84,    85,    86,    87,    88,    89,    90,
      -1,    -1,    -1,    -1,    -1,    96,    97,    44,    45,    46,
      47,    48,    49,    -1,    51,    52,    53,    54,    -1,    -1,
      -1,    -1,    59,    -1,    -1,    62,    -1,    -1,    65,    66,
      67,    -1,    -1,    -1,    71,    72,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,    85,    86,
      87,    88,    89,    90,    -1,    -1,    -1,    -1,    -1,    96,
      97,    44,    45,    46,    47,    48,    49,    -1,    51,    52,
      53,    54,    -1,    -1,    -1,    -1,    59,    -1,    -1,    62,
      -1,    -1,    65,    66,    67,    -1,    -1,    -1,    71,    72,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    84,    85,    86,    87,    88,    89,    90,    -1,    -1,
      -1,    -1,    -1,    96,    97,    44,    45,    46,    47,    48,
      49,    -1,    51,    52,    53,    54,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    62,    -1,    -1,    65,    66,    67,    -1,
      -1,    -1,    71,    72,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    84,    85,    86,    87,    88,
      89,    90,    -1,    -1,    -1,    -1,    -1,    96,    97,    44,
      45,    46,    47,    48,    49,    -1,    51,    52,    53,    54,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    62,    -1,    -1,
      65,    66,    67,    -1,    -1,    -1,    71,    72,    44,    45,
      46,    47,    48,    49,    -1,    51,    52,    53,    54,    84,
      85,    86,    87,    88,    89,    90,    62,    -1,    -1,    65,
      66,    67,    97,    -1,    -1,    71,    72,    44,    45,    46,
      47,    48,    49,    -1,    51,    52,    53,    54,    84,    85,
      86,    87,    88,    -1,    90,    62,    -1,    -1,    65,    66,
      67,    97,    -1,    -1,    71,    72,    44,    45,    46,    47,
      48,    49,    -1,    51,    -1,    53,    54,    84,    85,    86,
      87,    -1,    -1,    90,    62,    -1,    -1,    65,    66,    67,
      97,    -1,    -1,    71,    72,    44,    45,    46,    47,    48,
      49,    -1,    51,    -1,    53,    54,    84,    85,    86,    87,
      -1,    -1,    90,    -1,    -1,    -1,    65,    66,    67,    97,
      -1,    -1,    71,    72,    44,    45,    46,    47,    48,    49,
      -1,    51,    -1,    -1,    54,    84,    85,    86,    87,    -1,
      -1,    90,    -1,    -1,    -1,    65,    66,    67,    97,    -1,
      -1,    71,    72,    44,    45,    46,    47,    48,    49,    -1,
      51,    -1,    -1,    54,    84,    85,    86,    87,    -1,    -1,
      90,    -1,    -1,    -1,    65,    66,    67,    97,    -1,    -1,
      71,    72,    44,    45,    46,    47,    -1,    -1,    -1,    51,
      -1,    -1,    54,    50,    -1,    86,    87,    -1,    -1,    90,
      -1,    -1,    -1,    65,    66,    67,    97,    -1,    -1,    71,
      72,    68,    69,    50,    -1,    -1,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    -1,    -1,    90,    -1,
      -1,    68,    69,    -1,    -1,    97,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   101,   102,     0,     3,     5,     6,     7,     9,    11,
      13,    14,    15,    20,    21,    22,    24,    25,    27,    29,
      32,    35,    36,    37,    38,    40,    41,    42,    43,    45,
      46,    55,    63,    64,   103,   104,   107,   108,   111,   112,
     119,   122,   123,   124,   125,   126,   127,   128,   129,   132,
     133,   134,    38,    38,    55,   130,   130,    59,    59,    55,
       7,    36,    59,   127,   132,    55,    60,   107,   118,    55,
      55,    55,   130,    55,    55,    38,    55,    50,    50,    68,
      69,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    92,   131,    55,    91,   127,   127,   127,   127,   127,
      59,    44,    45,    46,    47,    48,    49,    51,    52,    53,
      54,    62,    65,    66,    67,    71,    72,    84,    85,    86,
      87,    88,    89,    90,    96,    97,    50,   131,    55,    91,
     127,    55,    55,   127,    59,   127,   106,    14,    59,   127,
      37,    37,    55,   127,   127,    60,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     140,   127,   135,   136,    38,    56,   127,   127,   127,   127,
     127,   127,    38,   127,   127,   127,   127,   127,   130,   130,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,    60,   127,    37,   109,   110,    38,    56,    92,   114,
     127,   114,    56,    56,    61,   107,    55,    59,   127,    59,
      23,    23,   127,    56,    56,   105,   108,    56,    57,    57,
      59,    57,    99,    56,    57,    55,    55,    92,    58,   136,
      56,    57,    55,   114,    58,   113,   113,   118,   118,   127,
      56,   127,    59,    59,   127,   127,   127,   113,    60,    60,
      61,   108,    41,   127,   127,    50,   131,   127,   135,   135,
     140,   127,    61,    60,    37,   109,    99,    38,    57,   115,
     115,     8,    56,   118,    56,    56,   127,    56,   127,    59,
      56,    56,    56,    26,   120,   120,    59,    56,    59,   127,
      56,    56,    99,   106,    56,   113,   136,    56,    56,   118,
     118,   118,    56,   118,    56,    56,   127,   118,   118,   113,
     121,   127,    61,    61,    61,    60,   115,    60,    60,   118,
     118,   118,    56,    60,    28,    58,   106,    56,    24,    38,
      39,   112,   116,   117,   138,   139,   116,   118,   137,   138,
     127,   106,    61,    60,    50,    50,    92,    38,    59,    61,
     112,   117,   139,    61,    61,    19,   120,   116,   127,   127,
     140,    50,    92,    59,    59,    58,    61,    59,    59,    99,
     127,   140,   106,    50,    59,    99,   127,    50,    59,   127,
      59
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,   100,   101,   102,   102,   103,   103,   103,   104,   105,
     105,   106,   106,   107,   107,   107,   107,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   107,   108,   108,   109,
     109,   110,   110,   111,   112,   112,   112,   112,   112,   112,
     113,   113,   114,   114,   115,   115,   116,   116,   116,   116,
     117,   117,   118,   118,   119,   119,   120,   120,   120,   121,
     121,   122,   122,   123,   123,   124,   124,   124,   124,   124,
     124,   124,   124,   125,   125,   126,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   128,   128,   129,   129,   130,
     130,   131,   131,   131,   131,   131,   131,   131,   131,   131,
     131,   131,   131,   132,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   133,   133,   133,   134,   134,   135,   135,
     136,   136,   137,   137,   138,   138,   139,   139,   139,   139,
     139,   140,   140
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     0,     2,     1,     1,     1,     6,     1,
       2,     0,     2,     1,     1,     1,     1,     1,     1,     2,
       2,     2,     3,     2,     4,     6,     1,     8,    10,     0,
       1,     1,     3,    11,    10,     7,    12,     9,    10,     7,
       0,     2,     0,     1,     0,     2,     0,     1,     1,     2,
       2,     3,     3,     1,     7,     7,     4,     7,     5,     1,
       3,     5,     7,     5,     6,     9,     8,     8,     7,     8,
       7,     7,     6,     7,     7,     1,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     2,     1,     5,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     4,     3,     6,     3,     3,     1,
       3,     1,     1,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     1,     1,     1,     3,     6,     2,     5,
       2,     3,     5,     4,     6,     6,     4,     6,     0,     1,
       1,     3,     0,     1,     1,     2,     4,     5,     4,     7,
       8,     1,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* start: decl_list  */
#line 162 "CMDgram.y"
      { }
#line 2012 "cmdgram.cpp"
    break;

  case 3: /* decl_list: %empty  */
#line 167 "CMDgram.y"
      { (yyval.stmt) = nil; }
#line 2018 "cmdgram.cpp"
    break;

  case 4: /* decl_list: decl_list decl  */
#line 169 "CMDgram.y"
      { if(!gStatementList) { gStatementList = (yyvsp[0].stmt); } else { gStatementList->append((yyvsp[0].stmt)); } }
#line 2024 "cmdgram.cpp"
    break;

  case 5: /* decl: stmt  */
#line 174 "CMDgram.y"
      { (yyval.stmt) = (yyvsp[0].stmt); }
#line 2030 "cmdgram.cpp"
    break;

  case 6: /* decl: fn_decl_stmt  */
#line 176 "CMDgram.y"
      { (yyval.stmt) = (yyvsp[0].stmt); }
#line 2036 "cmdgram.cpp"
    break;

  case 7: /* decl: package_decl  */
#line 178 "CMDgram.y"
     { (yyval.stmt) = (yyvsp[0].stmt); }
#line 2042 "cmdgram.cpp"
    break;

  case 8: /* package_decl: rwPACKAGE IDENT '{' fn_decl_list '}' ';'  */
#line 183 "CMDgram.y"
      { (yyval.stmt) = (yyvsp[-2].stmt); for(StmtNode *walk = ((yyvsp[-2].stmt));walk;walk = walk->getNext() ) walk->setPackage((yyvsp[-4].s).value); }
#line 2048 "cmdgram.cpp"
    break;

  case 9: /* fn_decl_list: fn_decl_stmt  */
#line 188 "CMDgram.y"
      { (yyval.stmt) = (yyvsp[0].stmt); }
#line 2054 "cmdgram.cpp"
    break;

  case 10: /* fn_decl_list: fn_decl_list fn_decl_stmt  */
#line 190 "CMDgram.y"
      { (yyval.stmt) = (yyvsp[-1].stmt); ((yyvsp[-1].stmt))->append((yyvsp[0].stmt));  }
#line 2060 "cmdgram.cpp"
    break;

  case 11: /* statement_list: %empty  */
#line 195 "CMDgram.y"
      { (yyval.stmt) = nil; }
#line 2066 "cmdgram.cpp"
    break;

  case 12: /* statement_list: statement_list stmt  */
#line 197 "CMDgram.y"
      { if(!(yyvsp[-1].stmt)) { (yyval.stmt) = (yyvsp[0].stmt); } else { ((yyvsp[-1].stmt))->append((yyvsp[0].stmt)); (yyval.stmt) = (yyvsp[-1].stmt); } }
#line 2072 "cmdgram.cpp"
    break;

  case 19: /* stmt: rwBREAK ';'  */
#line 208 "CMDgram.y"
      { (yyval.stmt) = BreakStmtNode::alloc( (yyvsp[-1].i).lineNumber ); }
#line 2078 "cmdgram.cpp"
    break;

  case 20: /* stmt: rwCONTINUE ';'  */
#line 210 "CMDgram.y"
      { (yyval.stmt) = ContinueStmtNode::alloc( (yyvsp[-1].i).lineNumber ); }
#line 2084 "cmdgram.cpp"
    break;

  case 21: /* stmt: rwRETURN ';'  */
#line 212 "CMDgram.y"
      { (yyval.stmt) = ReturnStmtNode::alloc( (yyvsp[-1].i).lineNumber, NULL ); }
#line 2090 "cmdgram.cpp"
    break;

  case 22: /* stmt: rwRETURN expr ';'  */
#line 214 "CMDgram.y"
      { (yyval.stmt) = ReturnStmtNode::alloc( (yyvsp[-2].i).lineNumber, (yyvsp[-1].expr) ); }
#line 2096 "cmdgram.cpp"
    break;

  case 23: /* stmt: expression_stmt ';'  */
#line 216 "CMDgram.y"
      { (yyval.stmt) = (yyvsp[-1].stmt); }
#line 2102 "cmdgram.cpp"
    break;

  case 24: /* stmt: TTAG '=' expr ';'  */
#line 218 "CMDgram.y"
      { (yyval.stmt) = TTagSetStmtNode::alloc( (yyvsp[-3].s).lineNumber, (yyvsp[-3].s).value, (yyvsp[-1].expr), NULL ); }
#line 2108 "cmdgram.cpp"
    break;

  case 25: /* stmt: TTAG '=' expr ',' expr ';'  */
#line 220 "CMDgram.y"
      { (yyval.stmt) = TTagSetStmtNode::alloc( (yyvsp[-5].s).lineNumber, (yyvsp[-5].s).value, (yyvsp[-3].expr), (yyvsp[-1].expr) ); }
#line 2114 "cmdgram.cpp"
    break;

  case 26: /* stmt: DOCBLOCK  */
#line 222 "CMDgram.y"
      { (yyval.stmt) = StrConstNode::alloc( (yyvsp[0].str).lineNumber, (yyvsp[0].str).value, false, true ); }
#line 2120 "cmdgram.cpp"
    break;

  case 27: /* fn_decl_stmt: rwDEFINE IDENT '(' var_list_decl ')' '{' statement_list '}'  */
#line 227 "CMDgram.y"
      { (yyval.stmt) = FunctionDeclStmtNode::alloc( (yyvsp[-7].i).lineNumber, (yyvsp[-6].s).value, NULL, (yyvsp[-4].var), (yyvsp[-1].stmt) ); }
#line 2126 "cmdgram.cpp"
    break;

  case 28: /* fn_decl_stmt: rwDEFINE IDENT opCOLONCOLON IDENT '(' var_list_decl ')' '{' statement_list '}'  */
#line 229 "CMDgram.y"
     { (yyval.stmt) = FunctionDeclStmtNode::alloc( (yyvsp[-9].i).lineNumber, (yyvsp[-6].s).value, (yyvsp[-8].s).value, (yyvsp[-4].var), (yyvsp[-1].stmt) ); }
#line 2132 "cmdgram.cpp"
    break;

  case 29: /* var_list_decl: %empty  */
#line 234 "CMDgram.y"
     { (yyval.var) = NULL; }
#line 2138 "cmdgram.cpp"
    break;

  case 30: /* var_list_decl: var_list  */
#line 236 "CMDgram.y"
     { (yyval.var) = (yyvsp[0].var); }
#line 2144 "cmdgram.cpp"
    break;

  case 31: /* var_list: VAR  */
#line 241 "CMDgram.y"
      { (yyval.var) = VarNode::alloc( (yyvsp[0].s).lineNumber, (yyvsp[0].s).value, NULL ); }
#line 2150 "cmdgram.cpp"
    break;

  case 32: /* var_list: var_list ',' VAR  */
#line 243 "CMDgram.y"
      { (yyval.var) = (yyvsp[-2].var); ((StmtNode*)((yyvsp[-2].var)))->append((StmtNode*)VarNode::alloc( (yyvsp[0].s).lineNumber, (yyvsp[0].s).value, NULL ) ); }
#line 2156 "cmdgram.cpp"
    break;

  case 33: /* datablock_decl: rwDATABLOCK class_name_expr '(' expr parent_block ')' parent_block '{' slot_assign_list_opt '}' ';'  */
#line 248 "CMDgram.y"
      { (yyval.stmt) = ObjectDeclNode::alloc( (yyvsp[-10].i).lineNumber, (yyvsp[-9].expr), (yyvsp[-7].expr), NULL, (yyvsp[-6].s).value ? (yyvsp[-6].s).value : (yyvsp[-4].s).value, (yyvsp[-2].slist), NULL, true, false, false); }
#line 2162 "cmdgram.cpp"
    break;

  case 34: /* object_decl: rwDECLARE class_name_expr '(' object_name parent_block object_args ')' '{' object_declare_block '}'  */
#line 253 "CMDgram.y"
      { (yyval.od) = ObjectDeclNode::alloc( (yyvsp[-9].i).lineNumber, (yyvsp[-8].expr), (yyvsp[-6].expr), (yyvsp[-4].expr), (yyvsp[-5].s).value, (yyvsp[-1].odcl).slots, (yyvsp[-1].odcl).decls, false, false, false); }
#line 2168 "cmdgram.cpp"
    break;

  case 35: /* object_decl: rwDECLARE class_name_expr '(' object_name parent_block object_args ')'  */
#line 255 "CMDgram.y"
      { (yyval.od) = ObjectDeclNode::alloc( (yyvsp[-6].i).lineNumber, (yyvsp[-5].expr), (yyvsp[-3].expr), (yyvsp[-1].expr), (yyvsp[-2].s).value, NULL, NULL, false, false, false); }
#line 2174 "cmdgram.cpp"
    break;

  case 36: /* object_decl: rwDECLARE class_name_expr '(' '[' object_name ']' parent_block object_args ')' '{' object_declare_block '}'  */
#line 257 "CMDgram.y"
      { (yyval.od) = ObjectDeclNode::alloc( (yyvsp[-11].i).lineNumber, (yyvsp[-10].expr), (yyvsp[-7].expr), (yyvsp[-4].expr), (yyvsp[-5].s).value, (yyvsp[-1].odcl).slots, (yyvsp[-1].odcl).decls, false, true, false); }
#line 2180 "cmdgram.cpp"
    break;

  case 37: /* object_decl: rwDECLARE class_name_expr '(' '[' object_name ']' parent_block object_args ')'  */
#line 259 "CMDgram.y"
      { (yyval.od) = ObjectDeclNode::alloc( (yyvsp[-8].i).lineNumber, (yyvsp[-7].expr), (yyvsp[-4].expr), (yyvsp[-1].expr), (yyvsp[-2].s).value, NULL, NULL, false, true, false); }
#line 2186 "cmdgram.cpp"
    break;

  case 38: /* object_decl: rwDECLARESINGLETON class_name_expr '(' object_name parent_block object_args ')' '{' object_declare_block '}'  */
#line 261 "CMDgram.y"
      { (yyval.od) = ObjectDeclNode::alloc( (yyvsp[-9].i).lineNumber, (yyvsp[-8].expr), (yyvsp[-6].expr), (yyvsp[-4].expr), (yyvsp[-5].s).value, (yyvsp[-1].odcl).slots, (yyvsp[-1].odcl).decls, false, false, true); }
#line 2192 "cmdgram.cpp"
    break;

  case 39: /* object_decl: rwDECLARESINGLETON class_name_expr '(' object_name parent_block object_args ')'  */
#line 263 "CMDgram.y"
      { (yyval.od) = ObjectDeclNode::alloc( (yyvsp[-6].i).lineNumber, (yyvsp[-5].expr), (yyvsp[-3].expr), (yyvsp[-1].expr), (yyvsp[-2].s).value, NULL, NULL, false, false, true); }
#line 2198 "cmdgram.cpp"
    break;

  case 40: /* parent_block: %empty  */
#line 268 "CMDgram.y"
      { (yyval.s).value = NULL; }
#line 2204 "cmdgram.cpp"
    break;

  case 41: /* parent_block: ':' IDENT  */
#line 270 "CMDgram.y"
      { (yyval.s) = (yyvsp[0].s); }
#line 2210 "cmdgram.cpp"
    break;

  case 42: /* object_name: %empty  */
#line 275 "CMDgram.y"
      { (yyval.expr) = StrConstNode::alloc( CodeBlock::smCurrentParser->getCurrentLine(), "", false); }
#line 2216 "cmdgram.cpp"
    break;

  case 43: /* object_name: expr  */
#line 277 "CMDgram.y"
      { (yyval.expr) = (yyvsp[0].expr); }
#line 2222 "cmdgram.cpp"
    break;

  case 44: /* object_args: %empty  */
#line 282 "CMDgram.y"
      { (yyval.expr) = NULL; }
#line 2228 "cmdgram.cpp"
    break;

  case 45: /* object_args: ',' expr_list  */
#line 284 "CMDgram.y"
      { (yyval.expr) = (yyvsp[0].expr); }
#line 2234 "cmdgram.cpp"
    break;

  case 46: /* object_declare_block: %empty  */
#line 289 "CMDgram.y"
      { (yyval.odcl).slots = NULL; (yyval.odcl).decls = NULL; }
#line 2240 "cmdgram.cpp"
    break;

  case 47: /* object_declare_block: slot_assign_list  */
#line 291 "CMDgram.y"
      { (yyval.odcl).slots = (yyvsp[0].slist); (yyval.odcl).decls = NULL; }
#line 2246 "cmdgram.cpp"
    break;

  case 48: /* object_declare_block: object_decl_list  */
#line 293 "CMDgram.y"
      { (yyval.odcl).slots = NULL; (yyval.odcl).decls = (yyvsp[0].od); }
#line 2252 "cmdgram.cpp"
    break;

  case 49: /* object_declare_block: slot_assign_list object_decl_list  */
#line 295 "CMDgram.y"
      { (yyval.odcl).slots = (yyvsp[-1].slist); (yyval.odcl).decls = (yyvsp[0].od); }
#line 2258 "cmdgram.cpp"
    break;

  case 50: /* object_decl_list: object_decl ';'  */
#line 300 "CMDgram.y"
      { (yyval.od) = (yyvsp[-1].od); }
#line 2264 "cmdgram.cpp"
    break;

  case 51: /* object_decl_list: object_decl_list object_decl ';'  */
#line 302 "CMDgram.y"
      { (yyvsp[-2].od)->append((yyvsp[-1].od)); (yyval.od) = (yyvsp[-2].od); }
#line 2270 "cmdgram.cpp"
    break;

  case 52: /* stmt_block: '{' statement_list '}'  */
#line 307 "CMDgram.y"
      { (yyval.stmt) = (yyvsp[-1].stmt); }
#line 2276 "cmdgram.cpp"
    break;

  case 53: /* stmt_block: stmt  */
#line 309 "CMDgram.y"
      { (yyval.stmt) = (yyvsp[0].stmt); }
#line 2282 "cmdgram.cpp"
    break;

  case 54: /* switch_stmt: rwSWITCH '(' expr ')' '{' case_block '}'  */
#line 314 "CMDgram.y"
      { (yyval.stmt) = (yyvsp[-1].ifnode); (yyvsp[-1].ifnode)->propagateSwitchExpr((yyvsp[-4].expr), false); }
#line 2288 "cmdgram.cpp"
    break;

  case 55: /* switch_stmt: rwSWITCHSTR '(' expr ')' '{' case_block '}'  */
#line 316 "CMDgram.y"
      { (yyval.stmt) = (yyvsp[-1].ifnode); (yyvsp[-1].ifnode)->propagateSwitchExpr((yyvsp[-4].expr), true); }
#line 2294 "cmdgram.cpp"
    break;

  case 56: /* case_block: rwCASE case_expr ':' statement_list  */
#line 321 "CMDgram.y"
      { (yyval.ifnode) = IfStmtNode::alloc( (yyvsp[-3].i).lineNumber, (yyvsp[-2].expr), (yyvsp[0].stmt), NULL, false); }
#line 2300 "cmdgram.cpp"
    break;

  case 57: /* case_block: rwCASE case_expr ':' statement_list rwDEFAULT ':' statement_list  */
#line 323 "CMDgram.y"
      { (yyval.ifnode) = IfStmtNode::alloc( (yyvsp[-6].i).lineNumber, (yyvsp[-5].expr), (yyvsp[-3].stmt), (yyvsp[0].stmt), false); }
#line 2306 "cmdgram.cpp"
    break;

  case 58: /* case_block: rwCASE case_expr ':' statement_list case_block  */
#line 325 "CMDgram.y"
      { (yyval.ifnode) = IfStmtNode::alloc( (yyvsp[-4].i).lineNumber, (yyvsp[-3].expr), (yyvsp[-1].stmt), (yyvsp[0].ifnode), true); }
#line 2312 "cmdgram.cpp"
    break;

  case 59: /* case_expr: expr  */
#line 330 "CMDgram.y"
      { (yyval.expr) = (yyvsp[0].expr);}
#line 2318 "cmdgram.cpp"
    break;

  case 60: /* case_expr: case_expr rwCASEOR expr  */
#line 332 "CMDgram.y"
      { ((yyvsp[-2].expr))->append((yyvsp[0].expr)); (yyval.expr)=(yyvsp[-2].expr); }
#line 2324 "cmdgram.cpp"
    break;

  case 61: /* if_stmt: rwIF '(' expr ')' stmt_block  */
#line 337 "CMDgram.y"
      { (yyval.stmt) = IfStmtNode::alloc((yyvsp[-4].i).lineNumber, (yyvsp[-2].expr), (yyvsp[0].stmt), NULL, false); }
#line 2330 "cmdgram.cpp"
    break;

  case 62: /* if_stmt: rwIF '(' expr ')' stmt_block rwELSE stmt_block  */
#line 339 "CMDgram.y"
      { (yyval.stmt) = IfStmtNode::alloc((yyvsp[-6].i).lineNumber, (yyvsp[-4].expr), (yyvsp[-2].stmt), (yyvsp[0].stmt), false); }
#line 2336 "cmdgram.cpp"
    break;

  case 63: /* while_stmt: rwWHILE '(' expr ')' stmt_block  */
#line 344 "CMDgram.y"
      { (yyval.stmt) = LoopStmtNode::alloc((yyvsp[-4].i).lineNumber, nil, (yyvsp[-2].expr), nil, (yyvsp[0].stmt), false); }
#line 2342 "cmdgram.cpp"
    break;

  case 64: /* while_stmt: rwDO stmt_block rwWHILE '(' expr ')'  */
#line 346 "CMDgram.y"
      { (yyval.stmt) = LoopStmtNode::alloc((yyvsp[-3].i).lineNumber, nil, (yyvsp[-1].expr), nil, (yyvsp[-4].stmt), true); }
#line 2348 "cmdgram.cpp"
    break;

  case 65: /* for_stmt: rwFOR '(' expr ';' expr ';' expr ')' stmt_block  */
#line 351 "CMDgram.y"
      { (yyval.stmt) = LoopStmtNode::alloc((yyvsp[-8].i).lineNumber, (yyvsp[-6].expr), (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].stmt), false); }
#line 2354 "cmdgram.cpp"
    break;

  case 66: /* for_stmt: rwFOR '(' expr ';' expr ';' ')' stmt_block  */
#line 353 "CMDgram.y"
      { (yyval.stmt) = LoopStmtNode::alloc((yyvsp[-7].i).lineNumber, (yyvsp[-5].expr), (yyvsp[-3].expr), NULL, (yyvsp[0].stmt), false); }
#line 2360 "cmdgram.cpp"
    break;

  case 67: /* for_stmt: rwFOR '(' expr ';' ';' expr ')' stmt_block  */
#line 355 "CMDgram.y"
      { (yyval.stmt) = LoopStmtNode::alloc((yyvsp[-7].i).lineNumber, (yyvsp[-5].expr), NULL, (yyvsp[-2].expr), (yyvsp[0].stmt), false); }
#line 2366 "cmdgram.cpp"
    break;

  case 68: /* for_stmt: rwFOR '(' expr ';' ';' ')' stmt_block  */
#line 357 "CMDgram.y"
      { (yyval.stmt) = LoopStmtNode::alloc((yyvsp[-6].i).lineNumber, (yyvsp[-4].expr), NULL, NULL, (yyvsp[0].stmt), false); }
#line 2372 "cmdgram.cpp"
    break;

  case 69: /* for_stmt: rwFOR '(' ';' expr ';' expr ')' stmt_block  */
#line 359 "CMDgram.y"
      { (yyval.stmt) = LoopStmtNode::alloc((yyvsp[-7].i).lineNumber, NULL, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].stmt), false); }
#line 2378 "cmdgram.cpp"
    break;

  case 70: /* for_stmt: rwFOR '(' ';' expr ';' ')' stmt_block  */
#line 361 "CMDgram.y"
      { (yyval.stmt) = LoopStmtNode::alloc((yyvsp[-6].i).lineNumber, NULL, (yyvsp[-3].expr), NULL, (yyvsp[0].stmt), false); }
#line 2384 "cmdgram.cpp"
    break;

  case 71: /* for_stmt: rwFOR '(' ';' ';' expr ')' stmt_block  */
#line 363 "CMDgram.y"
      { (yyval.stmt) = LoopStmtNode::alloc((yyvsp[-6].i).lineNumber, NULL, NULL, (yyvsp[-2].expr), (yyvsp[0].stmt), false); }
#line 2390 "cmdgram.cpp"
    break;

  case 72: /* for_stmt: rwFOR '(' ';' ';' ')' stmt_block  */
#line 365 "CMDgram.y"
      { (yyval.stmt) = LoopStmtNode::alloc((yyvsp[-5].i).lineNumber, NULL, NULL, NULL, (yyvsp[0].stmt), false); }
#line 2396 "cmdgram.cpp"
    break;

  case 73: /* foreach_stmt: rwFOREACH '(' VAR rwIN expr ')' stmt_block  */
#line 370 "CMDgram.y"
      { (yyval.stmt) = IterStmtNode::alloc( (yyvsp[-6].i).lineNumber, (yyvsp[-4].s).value, (yyvsp[-2].expr), (yyvsp[0].stmt), false ); }
#line 2402 "cmdgram.cpp"
    break;

  case 74: /* foreach_stmt: rwFOREACHSTR '(' VAR rwIN expr ')' stmt_block  */
#line 372 "CMDgram.y"
      { (yyval.stmt) = IterStmtNode::alloc( (yyvsp[-6].i).lineNumber, (yyvsp[-4].s).value, (yyvsp[-2].expr), (yyvsp[0].stmt), true ); }
#line 2408 "cmdgram.cpp"
    break;

  case 75: /* expression_stmt: stmt_expr  */
#line 377 "CMDgram.y"
      { (yyval.stmt) = (yyvsp[0].expr); }
#line 2414 "cmdgram.cpp"
    break;

  case 76: /* expr: stmt_expr  */
#line 382 "CMDgram.y"
      { (yyval.expr) = (yyvsp[0].expr); }
#line 2420 "cmdgram.cpp"
    break;

  case 77: /* expr: '(' expr ')'  */
#line 384 "CMDgram.y"
      { (yyval.expr) = (yyvsp[-1].expr); }
#line 2426 "cmdgram.cpp"
    break;

  case 78: /* expr: expr '^' expr  */
#line 386 "CMDgram.y"
      { (yyval.expr) = IntBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2432 "cmdgram.cpp"
    break;

  case 79: /* expr: expr '%' expr  */
#line 388 "CMDgram.y"
      { (yyval.expr) = IntBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2438 "cmdgram.cpp"
    break;

  case 80: /* expr: expr '&' expr  */
#line 390 "CMDgram.y"
      { (yyval.expr) = IntBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2444 "cmdgram.cpp"
    break;

  case 81: /* expr: expr '|' expr  */
#line 392 "CMDgram.y"
      { (yyval.expr) = IntBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2450 "cmdgram.cpp"
    break;

  case 82: /* expr: expr '+' expr  */
#line 394 "CMDgram.y"
      { (yyval.expr) = FloatBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2456 "cmdgram.cpp"
    break;

  case 83: /* expr: expr '-' expr  */
#line 396 "CMDgram.y"
      { (yyval.expr) = FloatBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2462 "cmdgram.cpp"
    break;

  case 84: /* expr: expr '*' expr  */
#line 398 "CMDgram.y"
      { (yyval.expr) = FloatBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2468 "cmdgram.cpp"
    break;

  case 85: /* expr: expr '/' expr  */
#line 400 "CMDgram.y"
      { (yyval.expr) = FloatBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2474 "cmdgram.cpp"
    break;

  case 86: /* expr: '-' expr  */
#line 402 "CMDgram.y"
      { (yyval.expr) = FloatUnaryExprNode::alloc( (yyvsp[-1].i).lineNumber, (yyvsp[-1].i).value, (yyvsp[0].expr)); }
#line 2480 "cmdgram.cpp"
    break;

  case 87: /* expr: '*' expr  */
#line 404 "CMDgram.y"
      { (yyval.expr) = TTagDerefNode::alloc( (yyvsp[-1].i).lineNumber, (yyvsp[0].expr) ); }
#line 2486 "cmdgram.cpp"
    break;

  case 88: /* expr: TTAG  */
#line 406 "CMDgram.y"
      { (yyval.expr) = TTagExprNode::alloc( (yyvsp[0].s).lineNumber, (yyvsp[0].s).value ); }
#line 2492 "cmdgram.cpp"
    break;

  case 89: /* expr: expr '?' expr ':' expr  */
#line 408 "CMDgram.y"
      { (yyval.expr) = ConditionalExprNode::alloc( (yyvsp[-4].expr)->dbgLineNumber, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2498 "cmdgram.cpp"
    break;

  case 90: /* expr: expr '<' expr  */
#line 410 "CMDgram.y"
      { (yyval.expr) = IntBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2504 "cmdgram.cpp"
    break;

  case 91: /* expr: expr '>' expr  */
#line 412 "CMDgram.y"
      { (yyval.expr) = IntBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2510 "cmdgram.cpp"
    break;

  case 92: /* expr: expr opGE expr  */
#line 414 "CMDgram.y"
      { (yyval.expr) = IntBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2516 "cmdgram.cpp"
    break;

  case 93: /* expr: expr opLE expr  */
#line 416 "CMDgram.y"
      { (yyval.expr) = IntBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2522 "cmdgram.cpp"
    break;

  case 94: /* expr: expr opEQ expr  */
#line 418 "CMDgram.y"
      { (yyval.expr) = IntBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2528 "cmdgram.cpp"
    break;

  case 95: /* expr: expr opNE expr  */
#line 420 "CMDgram.y"
      { (yyval.expr) = IntBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2534 "cmdgram.cpp"
    break;

  case 96: /* expr: expr opOR expr  */
#line 422 "CMDgram.y"
      { (yyval.expr) = IntBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2540 "cmdgram.cpp"
    break;

  case 97: /* expr: expr opSHL expr  */
#line 424 "CMDgram.y"
      { (yyval.expr) = IntBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2546 "cmdgram.cpp"
    break;

  case 98: /* expr: expr opSHR expr  */
#line 426 "CMDgram.y"
      { (yyval.expr) = IntBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2552 "cmdgram.cpp"
    break;

  case 99: /* expr: expr opAND expr  */
#line 428 "CMDgram.y"
      { (yyval.expr) = IntBinaryExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-1].i).value, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2558 "cmdgram.cpp"
    break;

  case 100: /* expr: expr opSTREQ expr  */
#line 430 "CMDgram.y"
      { (yyval.expr) = StreqExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-2].expr), (yyvsp[0].expr), true); }
#line 2564 "cmdgram.cpp"
    break;

  case 101: /* expr: expr opSTRNE expr  */
#line 432 "CMDgram.y"
      { (yyval.expr) = StreqExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-2].expr), (yyvsp[0].expr), false); }
#line 2570 "cmdgram.cpp"
    break;

  case 102: /* expr: expr '@' expr  */
#line 434 "CMDgram.y"
      { (yyval.expr) = StrcatExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-2].expr), (yyvsp[0].expr), (yyvsp[-1].i).value); }
#line 2576 "cmdgram.cpp"
    break;

  case 103: /* expr: '!' expr  */
#line 436 "CMDgram.y"
      { (yyval.expr) = IntUnaryExprNode::alloc((yyvsp[-1].i).lineNumber, (yyvsp[-1].i).value, (yyvsp[0].expr)); }
#line 2582 "cmdgram.cpp"
    break;

  case 104: /* expr: '~' expr  */
#line 438 "CMDgram.y"
      { (yyval.expr) = IntUnaryExprNode::alloc((yyvsp[-1].i).lineNumber, (yyvsp[-1].i).value, (yyvsp[0].expr)); }
#line 2588 "cmdgram.cpp"
    break;

  case 105: /* expr: TAGATOM  */
#line 440 "CMDgram.y"
      { (yyval.expr) = StrConstNode::alloc( (yyvsp[0].str).lineNumber, (yyvsp[0].str).value, true); }
#line 2594 "cmdgram.cpp"
    break;

  case 106: /* expr: FLTCONST  */
#line 442 "CMDgram.y"
      { (yyval.expr) = FloatNode::alloc( (yyvsp[0].f).lineNumber, (yyvsp[0].f).value ); }
#line 2600 "cmdgram.cpp"
    break;

  case 107: /* expr: INTCONST  */
#line 444 "CMDgram.y"
      { (yyval.expr) = IntNode::alloc( (yyvsp[0].i).lineNumber, (yyvsp[0].i).value ); }
#line 2606 "cmdgram.cpp"
    break;

  case 108: /* expr: rwBREAK  */
#line 446 "CMDgram.y"
      { (yyval.expr) = ConstantNode::alloc( (yyvsp[0].i).lineNumber, StringTable->insert("break")); }
#line 2612 "cmdgram.cpp"
    break;

  case 109: /* expr: slot_acc  */
#line 448 "CMDgram.y"
      { (yyval.expr) = SlotAccessNode::alloc( (yyvsp[0].slot).lineNumber, (yyvsp[0].slot).object, (yyvsp[0].slot).array, (yyvsp[0].slot).slotName ); }
#line 2618 "cmdgram.cpp"
    break;

  case 110: /* expr: intslot_acc  */
#line 450 "CMDgram.y"
      { (yyval.expr) = InternalSlotAccessNode::alloc( (yyvsp[0].intslot).lineNumber, (yyvsp[0].intslot).object, (yyvsp[0].intslot).slotExpr, (yyvsp[0].intslot).recurse); }
#line 2624 "cmdgram.cpp"
    break;

  case 111: /* expr: IDENT  */
#line 452 "CMDgram.y"
      { (yyval.expr) = ConstantNode::alloc( (yyvsp[0].s).lineNumber, (yyvsp[0].s).value ); }
#line 2630 "cmdgram.cpp"
    break;

  case 112: /* expr: STRATOM  */
#line 454 "CMDgram.y"
      { (yyval.expr) = StrConstNode::alloc( (yyvsp[0].str).lineNumber, (yyvsp[0].str).value, false); }
#line 2636 "cmdgram.cpp"
    break;

  case 113: /* expr: VAR  */
#line 456 "CMDgram.y"
      { (yyval.expr) = (ExprNode*)VarNode::alloc( (yyvsp[0].s).lineNumber, (yyvsp[0].s).value, NULL); }
#line 2642 "cmdgram.cpp"
    break;

  case 114: /* expr: VAR '[' aidx_expr ']'  */
#line 458 "CMDgram.y"
      { (yyval.expr) = (ExprNode*)VarNode::alloc( (yyvsp[-3].s).lineNumber, (yyvsp[-3].s).value, (yyvsp[-1].expr) ); }
#line 2648 "cmdgram.cpp"
    break;

  case 115: /* slot_acc: expr '.' IDENT  */
#line 463 "CMDgram.y"
      { (yyval.slot).lineNumber = (yyvsp[-2].expr)->dbgLineNumber; (yyval.slot).object = (yyvsp[-2].expr); (yyval.slot).slotName = (yyvsp[0].s).value; (yyval.slot).array = NULL; }
#line 2654 "cmdgram.cpp"
    break;

  case 116: /* slot_acc: expr '.' IDENT '[' aidx_expr ']'  */
#line 465 "CMDgram.y"
      { (yyval.slot).lineNumber = (yyvsp[-5].expr)->dbgLineNumber; (yyval.slot).object = (yyvsp[-5].expr); (yyval.slot).slotName = (yyvsp[-3].s).value; (yyval.slot).array = (yyvsp[-1].expr); }
#line 2660 "cmdgram.cpp"
    break;

  case 117: /* intslot_acc: expr opINTNAME class_name_expr  */
#line 470 "CMDgram.y"
     { (yyval.intslot).lineNumber = (yyvsp[-2].expr)->dbgLineNumber; (yyval.intslot).object = (yyvsp[-2].expr); (yyval.intslot).slotExpr = (yyvsp[0].expr); (yyval.intslot).recurse = false; }
#line 2666 "cmdgram.cpp"
    break;

  case 118: /* intslot_acc: expr opINTNAMER class_name_expr  */
#line 472 "CMDgram.y"
     { (yyval.intslot).lineNumber = (yyvsp[-2].expr)->dbgLineNumber; (yyval.intslot).object = (yyvsp[-2].expr); (yyval.intslot).slotExpr = (yyvsp[0].expr); (yyval.intslot).recurse = true; }
#line 2672 "cmdgram.cpp"
    break;

  case 119: /* class_name_expr: IDENT  */
#line 477 "CMDgram.y"
      { (yyval.expr) = ConstantNode::alloc( (yyvsp[0].s).lineNumber, (yyvsp[0].s).value ); }
#line 2678 "cmdgram.cpp"
    break;

  case 120: /* class_name_expr: '(' expr ')'  */
#line 479 "CMDgram.y"
      { (yyval.expr) = (yyvsp[-1].expr); }
#line 2684 "cmdgram.cpp"
    break;

  case 121: /* assign_op_struct: opPLUSPLUS  */
#line 484 "CMDgram.y"
      { (yyval.asn).lineNumber = (yyvsp[0].i).lineNumber; (yyval.asn).token = '+'; (yyval.asn).expr = FloatNode::alloc( (yyvsp[0].i).lineNumber, 1 ); }
#line 2690 "cmdgram.cpp"
    break;

  case 122: /* assign_op_struct: opMINUSMINUS  */
#line 486 "CMDgram.y"
      { (yyval.asn).lineNumber = (yyvsp[0].i).lineNumber; (yyval.asn).token = '-'; (yyval.asn).expr = FloatNode::alloc( (yyvsp[0].i).lineNumber, 1 ); }
#line 2696 "cmdgram.cpp"
    break;

  case 123: /* assign_op_struct: opPLASN expr  */
#line 488 "CMDgram.y"
      { (yyval.asn).lineNumber = (yyvsp[-1].i).lineNumber; (yyval.asn).token = '+'; (yyval.asn).expr = (yyvsp[0].expr); }
#line 2702 "cmdgram.cpp"
    break;

  case 124: /* assign_op_struct: opMIASN expr  */
#line 490 "CMDgram.y"
      { (yyval.asn).lineNumber = (yyvsp[-1].i).lineNumber; (yyval.asn).token = '-'; (yyval.asn).expr = (yyvsp[0].expr); }
#line 2708 "cmdgram.cpp"
    break;

  case 125: /* assign_op_struct: opMLASN expr  */
#line 492 "CMDgram.y"
      { (yyval.asn).lineNumber = (yyvsp[-1].i).lineNumber; (yyval.asn).token = '*'; (yyval.asn).expr = (yyvsp[0].expr); }
#line 2714 "cmdgram.cpp"
    break;

  case 126: /* assign_op_struct: opDVASN expr  */
#line 494 "CMDgram.y"
      { (yyval.asn).lineNumber = (yyvsp[-1].i).lineNumber; (yyval.asn).token = '/'; (yyval.asn).expr = (yyvsp[0].expr); }
#line 2720 "cmdgram.cpp"
    break;

  case 127: /* assign_op_struct: opMODASN expr  */
#line 496 "CMDgram.y"
      { (yyval.asn).lineNumber = (yyvsp[-1].i).lineNumber; (yyval.asn).token = '%'; (yyval.asn).expr = (yyvsp[0].expr); }
#line 2726 "cmdgram.cpp"
    break;

  case 128: /* assign_op_struct: opANDASN expr  */
#line 498 "CMDgram.y"
      { (yyval.asn).lineNumber = (yyvsp[-1].i).lineNumber; (yyval.asn).token = '&'; (yyval.asn).expr = (yyvsp[0].expr); }
#line 2732 "cmdgram.cpp"
    break;

  case 129: /* assign_op_struct: opXORASN expr  */
#line 500 "CMDgram.y"
      { (yyval.asn).lineNumber = (yyvsp[-1].i).lineNumber; (yyval.asn).token = '^'; (yyval.asn).expr = (yyvsp[0].expr); }
#line 2738 "cmdgram.cpp"
    break;

  case 130: /* assign_op_struct: opORASN expr  */
#line 502 "CMDgram.y"
      { (yyval.asn).lineNumber = (yyvsp[-1].i).lineNumber; (yyval.asn).token = '|'; (yyval.asn).expr = (yyvsp[0].expr); }
#line 2744 "cmdgram.cpp"
    break;

  case 131: /* assign_op_struct: opSLASN expr  */
#line 504 "CMDgram.y"
      { (yyval.asn).lineNumber = (yyvsp[-1].i).lineNumber; (yyval.asn).token = opSHL; (yyval.asn).expr = (yyvsp[0].expr); }
#line 2750 "cmdgram.cpp"
    break;

  case 132: /* assign_op_struct: opSRASN expr  */
#line 506 "CMDgram.y"
      { (yyval.asn).lineNumber = (yyvsp[-1].i).lineNumber; (yyval.asn).token = opSHR; (yyval.asn).expr = (yyvsp[0].expr); }
#line 2756 "cmdgram.cpp"
    break;

  case 133: /* stmt_expr: funcall_expr  */
#line 511 "CMDgram.y"
      { (yyval.expr) = (yyvsp[0].expr); }
#line 2762 "cmdgram.cpp"
    break;

  case 134: /* stmt_expr: assert_expr  */
#line 513 "CMDgram.y"
      { (yyval.expr) = (yyvsp[0].expr); }
#line 2768 "cmdgram.cpp"
    break;

  case 135: /* stmt_expr: object_decl  */
#line 515 "CMDgram.y"
      { (yyval.expr) = (yyvsp[0].od); }
#line 2774 "cmdgram.cpp"
    break;

  case 136: /* stmt_expr: VAR '=' expr  */
#line 517 "CMDgram.y"
      { (yyval.expr) = AssignExprNode::alloc( (yyvsp[-2].s).lineNumber, (yyvsp[-2].s).value, NULL, (yyvsp[0].expr)); }
#line 2780 "cmdgram.cpp"
    break;

  case 137: /* stmt_expr: VAR '[' aidx_expr ']' '=' expr  */
#line 519 "CMDgram.y"
      { (yyval.expr) = AssignExprNode::alloc( (yyvsp[-5].s).lineNumber, (yyvsp[-5].s).value, (yyvsp[-3].expr), (yyvsp[0].expr)); }
#line 2786 "cmdgram.cpp"
    break;

  case 138: /* stmt_expr: VAR assign_op_struct  */
#line 521 "CMDgram.y"
      { (yyval.expr) = AssignOpExprNode::alloc( (yyvsp[-1].s).lineNumber, (yyvsp[-1].s).value, NULL, (yyvsp[0].asn).expr, (yyvsp[0].asn).token); }
#line 2792 "cmdgram.cpp"
    break;

  case 139: /* stmt_expr: VAR '[' aidx_expr ']' assign_op_struct  */
#line 523 "CMDgram.y"
      { (yyval.expr) = AssignOpExprNode::alloc( (yyvsp[-4].s).lineNumber, (yyvsp[-4].s).value, (yyvsp[-2].expr), (yyvsp[0].asn).expr, (yyvsp[0].asn).token); }
#line 2798 "cmdgram.cpp"
    break;

  case 140: /* stmt_expr: slot_acc assign_op_struct  */
#line 525 "CMDgram.y"
      { (yyval.expr) = SlotAssignOpNode::alloc( (yyvsp[-1].slot).lineNumber, (yyvsp[-1].slot).object, (yyvsp[-1].slot).slotName, (yyvsp[-1].slot).array, (yyvsp[0].asn).token, (yyvsp[0].asn).expr); }
#line 2804 "cmdgram.cpp"
    break;

  case 141: /* stmt_expr: slot_acc '=' expr  */
#line 527 "CMDgram.y"
      { (yyval.expr) = SlotAssignNode::alloc( (yyvsp[-2].slot).lineNumber, (yyvsp[-2].slot).object, (yyvsp[-2].slot).array, (yyvsp[-2].slot).slotName, (yyvsp[0].expr)); }
#line 2810 "cmdgram.cpp"
    break;

  case 142: /* stmt_expr: slot_acc '=' '{' expr_list '}'  */
#line 529 "CMDgram.y"
      { (yyval.expr) = SlotAssignNode::alloc( (yyvsp[-4].slot).lineNumber, (yyvsp[-4].slot).object, (yyvsp[-4].slot).array, (yyvsp[-4].slot).slotName, (yyvsp[-1].expr)); }
#line 2816 "cmdgram.cpp"
    break;

  case 143: /* funcall_expr: IDENT '(' expr_list_decl ')'  */
#line 534 "CMDgram.y"
     { (yyval.expr) = FuncCallExprNode::alloc( (yyvsp[-3].s).lineNumber, (yyvsp[-3].s).value, NULL, (yyvsp[-1].expr), false); }
#line 2822 "cmdgram.cpp"
    break;

  case 144: /* funcall_expr: IDENT opCOLONCOLON IDENT '(' expr_list_decl ')'  */
#line 536 "CMDgram.y"
     { (yyval.expr) = FuncCallExprNode::alloc( (yyvsp[-5].s).lineNumber, (yyvsp[-3].s).value, (yyvsp[-5].s).value, (yyvsp[-1].expr), false); }
#line 2828 "cmdgram.cpp"
    break;

  case 145: /* funcall_expr: expr '.' IDENT '(' expr_list_decl ')'  */
#line 538 "CMDgram.y"
      { (yyvsp[-5].expr)->append((yyvsp[-1].expr)); (yyval.expr) = FuncCallExprNode::alloc( (yyvsp[-5].expr)->dbgLineNumber, (yyvsp[-3].s).value, NULL, (yyvsp[-5].expr), true); }
#line 2834 "cmdgram.cpp"
    break;

  case 146: /* assert_expr: rwASSERT '(' expr ')'  */
#line 543 "CMDgram.y"
      { (yyval.expr) = AssertCallExprNode::alloc( (yyvsp[-3].i).lineNumber, (yyvsp[-1].expr), NULL ); }
#line 2840 "cmdgram.cpp"
    break;

  case 147: /* assert_expr: rwASSERT '(' expr ',' STRATOM ')'  */
#line 545 "CMDgram.y"
      { (yyval.expr) = AssertCallExprNode::alloc( (yyvsp[-5].i).lineNumber, (yyvsp[-3].expr), (yyvsp[-1].str).value ); }
#line 2846 "cmdgram.cpp"
    break;

  case 148: /* expr_list_decl: %empty  */
#line 550 "CMDgram.y"
      { (yyval.expr) = NULL; }
#line 2852 "cmdgram.cpp"
    break;

  case 149: /* expr_list_decl: expr_list  */
#line 552 "CMDgram.y"
      { (yyval.expr) = (yyvsp[0].expr); }
#line 2858 "cmdgram.cpp"
    break;

  case 150: /* expr_list: expr  */
#line 557 "CMDgram.y"
      { (yyval.expr) = (yyvsp[0].expr); }
#line 2864 "cmdgram.cpp"
    break;

  case 151: /* expr_list: expr_list ',' expr  */
#line 559 "CMDgram.y"
      { ((yyvsp[-2].expr))->append((yyvsp[0].expr)); (yyval.expr) = (yyvsp[-2].expr); }
#line 2870 "cmdgram.cpp"
    break;

  case 152: /* slot_assign_list_opt: %empty  */
#line 564 "CMDgram.y"
      { (yyval.slist) = NULL; }
#line 2876 "cmdgram.cpp"
    break;

  case 153: /* slot_assign_list_opt: slot_assign_list  */
#line 566 "CMDgram.y"
      { (yyval.slist) = (yyvsp[0].slist); }
#line 2882 "cmdgram.cpp"
    break;

  case 154: /* slot_assign_list: slot_assign  */
#line 571 "CMDgram.y"
      { (yyval.slist) = (yyvsp[0].slist); }
#line 2888 "cmdgram.cpp"
    break;

  case 155: /* slot_assign_list: slot_assign_list slot_assign  */
#line 573 "CMDgram.y"
      { (yyvsp[-1].slist)->append((yyvsp[0].slist)); (yyval.slist) = (yyvsp[-1].slist); }
#line 2894 "cmdgram.cpp"
    break;

  case 156: /* slot_assign: IDENT '=' expr ';'  */
#line 578 "CMDgram.y"
      { (yyval.slist) = SlotAssignNode::alloc( (yyvsp[-3].s).lineNumber, NULL, NULL, (yyvsp[-3].s).value, (yyvsp[-1].expr)); }
#line 2900 "cmdgram.cpp"
    break;

  case 157: /* slot_assign: TYPEIDENT IDENT '=' expr ';'  */
#line 580 "CMDgram.y"
      { (yyval.slist) = SlotAssignNode::alloc( (yyvsp[-4].i).lineNumber, NULL, NULL, (yyvsp[-3].s).value, (yyvsp[-1].expr), (yyvsp[-4].i).value); }
#line 2906 "cmdgram.cpp"
    break;

  case 158: /* slot_assign: rwDATABLOCK '=' expr ';'  */
#line 582 "CMDgram.y"
      { (yyval.slist) = SlotAssignNode::alloc( (yyvsp[-3].i).lineNumber, NULL, NULL, StringTable->insert("datablock"), (yyvsp[-1].expr)); }
#line 2912 "cmdgram.cpp"
    break;

  case 159: /* slot_assign: IDENT '[' aidx_expr ']' '=' expr ';'  */
#line 584 "CMDgram.y"
      { (yyval.slist) = SlotAssignNode::alloc( (yyvsp[-6].s).lineNumber, NULL, (yyvsp[-4].expr), (yyvsp[-6].s).value, (yyvsp[-1].expr)); }
#line 2918 "cmdgram.cpp"
    break;

  case 160: /* slot_assign: TYPEIDENT IDENT '[' aidx_expr ']' '=' expr ';'  */
#line 586 "CMDgram.y"
      { (yyval.slist) = SlotAssignNode::alloc( (yyvsp[-7].i).lineNumber, NULL, (yyvsp[-4].expr), (yyvsp[-6].s).value, (yyvsp[-1].expr), (yyvsp[-7].i).value); }
#line 2924 "cmdgram.cpp"
    break;

  case 161: /* aidx_expr: expr  */
#line 591 "CMDgram.y"
      { (yyval.expr) = (yyvsp[0].expr); }
#line 2930 "cmdgram.cpp"
    break;

  case 162: /* aidx_expr: aidx_expr ',' expr  */
#line 593 "CMDgram.y"
      { (yyval.expr) = CommaCatExprNode::alloc( (yyvsp[-2].expr)->dbgLineNumber, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2936 "cmdgram.cpp"
    break;


#line 2940 "cmdgram.cpp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 595 "CMDgram.y"


