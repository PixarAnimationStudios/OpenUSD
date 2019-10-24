//
// Copyright 2019 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 1

/* Substitute the variable and function names.  */
#define yyparse         virtualStructConditionalGrammarYyparse
#define yylex           virtualStructConditionalGrammarYylex
#define yyerror         virtualStructConditionalGrammarYyerror
#define yylval          virtualStructConditionalGrammarYylval
#define yychar          virtualStructConditionalGrammarYychar
#define yydebug         virtualStructConditionalGrammarYydebug
#define yynerrs         virtualStructConditionalGrammarYynerrs
#define yylloc          virtualStructConditionalGrammarYylloc

/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 30 "hdPrman/virtualStructConditionalGrammar.yy"


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <math.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "hdPrman/debugCodes.h"
#include "hdPrman/matfiltResolveVstructs.h"

#include "pxr/pxr.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"

PXR_NAMESPACE_USING_DIRECTIVE

// ---------------------------------------------------------------------------

struct _VSCGConditionalBase
{
    virtual ~_VSCGConditionalBase() = default;
    
    typedef std::shared_ptr<_VSCGConditionalBase> Ptr;

    virtual bool Eval(const MatfiltNode & node,
            const NdrTokenVec & shaderTypePriority) = 0;
};

// ---------------------------------------------------------------------------

namespace // anonymous
{

struct ConditionalParamBase : public _VSCGConditionalBase
{
    ConditionalParamBase(const TfToken & name)
    : paramName(name)
    {}

    TfToken paramName;
};

struct ConditionalParamIsConnected : public ConditionalParamBase
{
    ConditionalParamIsConnected(const TfToken & name)
    : ConditionalParamBase(name) {}

    bool Eval(const MatfiltNode & node, const NdrTokenVec & shaderTypePriority)
            override {
        
        auto I = node.inputConnections.find(paramName);
        if (I == node.inputConnections.end()) {
            return false;
        }

        return !(*I).second.empty();
    }
};

struct ConditionalParamIsNotConnected : public ConditionalParamBase
{
    ConditionalParamIsNotConnected(const TfToken & name)
    : ConditionalParamBase(name) {}

    bool Eval(const MatfiltNode & node,
            const NdrTokenVec & shaderTypePriority) override {
        auto I = node.inputConnections.find(paramName);
        if (I == node.inputConnections.end()) {
            return true;
        }

        return (*I).second.empty();
    }
};

struct ConditionalParamIsSet : public ConditionalParamBase
{
    ConditionalParamIsSet(const TfToken & name)
    : ConditionalParamBase(name) {}

    bool Eval(const MatfiltNode & node, const NdrTokenVec & shaderTypePriority)
            override {
        return (node.parameters.find(paramName)
                != node.parameters.end());
    }
};

struct ConditionalParamIsNotSet : public ConditionalParamBase
{
    ConditionalParamIsNotSet(const TfToken & name)
    : ConditionalParamBase(name) {}

    bool Eval(const MatfiltNode & node, const NdrTokenVec & shaderTypePriority)
            override {
        return (node.parameters.find(paramName)
                == node.parameters.end());
    }
};

struct ConditionalParamCmpBase : public ConditionalParamBase
{
    ConditionalParamCmpBase(const TfToken & name, const VtValue & v)
    : ConditionalParamBase(name) , value(v) {}

    VtValue value;

    static bool ValueAsNumber(const VtValue v, double * result) {
        if (v.IsHolding<float>()) {
            *result = v.UncheckedGet<float>();
            return true;
        }
        
        if (v.IsHolding<int>()) {
            *result = v.UncheckedGet<int>();
            return true;
        }

        if (v.IsHolding<double>()) {
            *result = v.UncheckedGet<double>();
            return true;
        }

        if (v.IsHolding<bool>()) {
            *result = v.UncheckedGet<bool>();
            return true;
        }

        // TODO check color?

        return false;
    }

    static bool GetParameterValue(
            const MatfiltNode & node,
            const TfToken & paramName,
            const NdrTokenVec & shaderTypePriority,
            VtValue * result) {

        auto I = node.parameters.find(paramName);
        if (I != node.parameters.end()) {
            *result = (*I).second;
            return true;
        }

        // check for a default
        auto& reg = SdrRegistry::GetInstance();

        if (SdrShaderNodeConstPtr sdrNode = reg.GetShaderNodeByIdentifier(
                node.nodeTypeId, shaderTypePriority)) {
            if (NdrPropertyConstPtr ndrProp = sdrNode->GetInput(paramName)) {
                *result = ndrProp->GetDefaultValue();
                return true;
            }
        }

        return false;
    }

    virtual bool CompareNumber(double v1, double v2) = 0;
    virtual bool CompareString(
            const std::string & v1, const std::string & v2) = 0;

    bool Eval(const MatfiltNode & node,
            const NdrTokenVec & shaderTypePriority) override {
        VtValue paramValue;
        if (!GetParameterValue(
                node, paramName, shaderTypePriority, &paramValue)) {
            return false;
        }

        if (value.IsHolding<std::string>()) {
            if (paramValue.IsHolding<std::string>()) {
                return CompareString(value.UncheckedGet<std::string>(),
                        paramValue.UncheckedGet<std::string>());
            }
            return false;
        }

        double d1 = 0, d2 = 0;

        if (ValueAsNumber(paramValue, &d1) && ValueAsNumber(value, &d2)) {    
            
            //std::cerr << d1 << " " << d2 << std::endl;
            return CompareNumber(d1, d2);
        }

        return false;
    }

};


struct ConditionalParamCmpEqualTo : public ConditionalParamCmpBase
{
    ConditionalParamCmpEqualTo(const TfToken & name, const VtValue & v)
    : ConditionalParamCmpBase(name, v) {}
    
    bool CompareNumber(double v1, double v2) override {
        return v1 == v2;
    }

    bool CompareString(const std::string & v1, const std::string & v2)
            override {
        return v1 == v2;
    }
};

struct ConditionalParamCmpNotEqualTo : public ConditionalParamCmpBase
{
    ConditionalParamCmpNotEqualTo(const TfToken & name, const VtValue & v)
    : ConditionalParamCmpBase(name, v) {}

    bool CompareNumber(double v1, double v2) override {
        return v1 != v2;
    }

    bool CompareString(const std::string & v1, const std::string & v2)
            override {
        return v1 != v2;
    }
};


struct ConditionalParamCmpGreaterThan : public ConditionalParamCmpBase
{
    ConditionalParamCmpGreaterThan(const TfToken & name, const VtValue & v)
    : ConditionalParamCmpBase(name, v) {
    }

    bool CompareNumber(double v1, double v2) override {
        return v1 > v2;
    }

    bool CompareString(const std::string & v1, const std::string & v2)
            override {
        return false;
    }
};

struct ConditionalParamCmpLessThan : public ConditionalParamCmpBase
{
    ConditionalParamCmpLessThan(const TfToken & name, const VtValue & v)
    : ConditionalParamCmpBase(name, v) {}

    bool CompareNumber(double v1, double v2) override {
        return v1 < v2;
    }

    bool CompareString(const std::string & v1, const std::string & v2)
            override {
        return false;
    }
};

struct ConditionalParamCmpGreaterThanOrEqualTo
    : public ConditionalParamCmpBase
{
    ConditionalParamCmpGreaterThanOrEqualTo(
            const TfToken & name, const VtValue & v)
    : ConditionalParamCmpBase(name, v) {}

    bool CompareNumber(double v1, double v2) override {
        return v1 >= v2;
    }

    bool CompareString(const std::string & v1, const std::string & v2)
            override {
        return false;
    }
};


struct ConditionalParamCmpLessThanOrEqualTo : public ConditionalParamCmpBase
{
    ConditionalParamCmpLessThanOrEqualTo(
            const TfToken & name, const VtValue & v)
    : ConditionalParamCmpBase(name, v) {}

    bool CompareNumber(double v1, double v2) override {
        return v1 <= v2;
    }

    bool CompareString(const std::string & v1, const std::string & v2)
            override {
        return false;
    }
};

struct ConditionalAnd : _VSCGConditionalBase
{
    ConditionalAnd(_VSCGConditionalBase * left, _VSCGConditionalBase * right)
    : _VSCGConditionalBase(), left(left), right(right) {}

    bool Eval(const MatfiltNode & node,
            const NdrTokenVec & shaderTypePriority) override {

        return (left && left->Eval(node, shaderTypePriority))
                && (right && right->Eval(node, shaderTypePriority)); 

    }

    ~ConditionalAnd() {
        delete left;
        delete right;
    }

    _VSCGConditionalBase * left;
    _VSCGConditionalBase * right;
};

struct ConditionalOr : _VSCGConditionalBase
{
    ConditionalOr(_VSCGConditionalBase * left, _VSCGConditionalBase * right)
    : _VSCGConditionalBase(), left(left), right(right) {}

    ~ConditionalOr() {
        delete left;
        delete right;
    }
    bool Eval(const MatfiltNode & node, const NdrTokenVec & shaderTypePriority)
            override {
        return (left && left->Eval(node, shaderTypePriority))
                || (right && right->Eval(node, shaderTypePriority)); 
    }

    _VSCGConditionalBase * left;
    _VSCGConditionalBase * right;
};


//----------------------------------------------------------------------------


} // namespace anonymous


struct _VSCGValueContainer
{
    _VSCGValueContainer(const VtValue & v)
    : value(v) {}

    VtValue value;
};

struct _VSCGAction {

    enum Action
    {
        Connect,
        Ignore,
        SetConstant,
        CopyParam,
    };

    _VSCGAction(Action action)
    : action(action) {}
    _VSCGAction(Action action, const VtValue & value)
    : action(action), value(value) {}

    Action action;
    VtValue value;
};


struct _VSCGParserData {

    // flex scanner
    void * yyscanner;
    std::string parseError;

    _VSCGAction * action = nullptr;
    _VSCGAction * fallbackAction = nullptr;
    
    std::set<_VSCGConditionalBase *> _intermediateConditions;

    _VSCGConditionalBase * NewCondition(_VSCGConditionalBase * condition) {
        _intermediateConditions.insert(condition);
        return condition;
    }

    _VSCGConditionalBase * ClaimCondition(_VSCGConditionalBase * condition) {
        _intermediateConditions.erase(condition);
        return condition;
    }

    _VSCGConditionalBase * rootCondition = nullptr;

    ~_VSCGParserData() {
        delete action;
        delete fallbackAction;

        for (auto c : _intermediateConditions) {
            delete c;
        }
        _intermediateConditions.clear();
    }
};

PXR_NAMESPACE_OPEN_SCOPE

class MatfiltVstructConditionalEvaluatorImpl
{
public:

    ~MatfiltVstructConditionalEvaluatorImpl() {
        delete condition;
        delete action;
        delete fallbackAction;
    }

    _VSCGConditionalBase * condition = nullptr;
    _VSCGAction * action = nullptr;
    _VSCGAction * fallbackAction = nullptr;
    
};

PXR_NAMESPACE_CLOSE_SCOPE

#include "virtualStructConditionalGrammar.tab.h"
 


/* Line 189 of yacc.c  */
#line 508 "hdPrman/virtualStructConditionalGrammar.tab.cpp"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NUMBER = 258,
     STRING = 259,
     PARAM = 260,
     LPAR = 261,
     RPAR = 262,
     OP_EQ = 263,
     OP_NOTEQ = 264,
     OP_GT = 265,
     OP_LT = 266,
     OP_GTEQ = 267,
     OP_LTEQ = 268,
     OP_IS = 269,
     OP_ISNOT = 270,
     OP_AND = 271,
     OP_OR = 272,
     KW_IF = 273,
     KW_ELSE = 274,
     KW_CONNECTED = 275,
     KW_CONNECT = 276,
     KW_IGNORE = 277,
     KW_COPY = 278,
     KW_SET = 279,
     UNRECOGNIZED_TOKEN = 280
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 456 "hdPrman/virtualStructConditionalGrammar.yy"

  double number;
  char * string;
  struct _VSCGConditionalBase * condition;
  struct _VSCGAction * action;
  struct _VSCGValueContainer * value;



/* Line 214 of yacc.c  */
#line 579 "hdPrman/virtualStructConditionalGrammar.tab.cpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */

/* Line 264 of yacc.c  */
#line 489 "hdPrman/virtualStructConditionalGrammar.yy"

int virtualStructConditionalGrammarYylex(
        YYSTYPE * lvalp, YYLTYPE *llocp, void * yyscanner);
void virtualStructConditionalGrammarYyerror(
        YYLTYPE *llocp, _VSCGParserData * data, const char *s);

/*This macro is to hack around the fact that
%lex-param ends up getting cut off, for example:
%lex-param { void * data->yyscanner}
Really ends up passing in yyscanner in the YYLEX macro
*/
#define MACRO_YYSCANNER data->yyscanner


/* Line 264 of yacc.c  */
#line 619 "hdPrman/virtualStructConditionalGrammar.tab.cpp"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

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
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  117
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   328

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  26
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  5
/* YYNRULES -- Number of rules.  */
#define YYNRULES  126
/* YYNRULES -- Number of states.  */
#define YYNSTATES  243

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   280

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
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
      25
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,    11,    15,    19,    23,    27,
      31,    35,    39,    43,    47,    51,    55,    59,    63,    67,
      71,    75,    79,    83,    87,    91,    95,    99,   103,   107,
     111,   115,   119,   123,   127,   131,   135,   139,   143,   147,
     151,   155,   159,   163,   167,   171,   175,   179,   183,   187,
     191,   195,   199,   203,   207,   211,   215,   219,   223,   227,
     231,   235,   239,   243,   247,   251,   255,   259,   263,   267,
     271,   275,   279,   283,   287,   291,   295,   299,   303,   307,
     311,   315,   319,   323,   327,   331,   335,   339,   343,   347,
     351,   355,   359,   363,   367,   371,   375,   379,   383,   387,
     391,   395,   399,   403,   407,   411,   415,   419,   423,   427,
     431,   435,   439,   443,   447,   451,   455,   459,   462,   464,
     466,   469,   472,   478,   482,   487,   489
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      30,     0,    -1,     4,    -1,     3,    -1,     5,     8,    27,
      -1,    16,     8,    27,    -1,    17,     8,    27,    -1,    14,
       8,    27,    -1,    18,     8,    27,    -1,    19,     8,    27,
      -1,    20,     8,    27,    -1,    21,     8,    27,    -1,    22,
       8,    27,    -1,    23,     8,    27,    -1,    24,     8,    27,
      -1,     5,     9,    27,    -1,    16,     9,    27,    -1,    17,
       9,    27,    -1,    14,     9,    27,    -1,    18,     9,    27,
      -1,    19,     9,    27,    -1,    20,     9,    27,    -1,    21,
       9,    27,    -1,    22,     9,    27,    -1,    23,     9,    27,
      -1,    24,     9,    27,    -1,     5,    10,    27,    -1,    16,
      10,    27,    -1,    17,    10,    27,    -1,    14,    10,    27,
      -1,    18,    10,    27,    -1,    19,    10,    27,    -1,    20,
      10,    27,    -1,    21,    10,    27,    -1,    22,    10,    27,
      -1,    23,    10,    27,    -1,    24,    10,    27,    -1,     5,
      11,    27,    -1,    16,    11,    27,    -1,    17,    11,    27,
      -1,    14,    11,    27,    -1,    18,    11,    27,    -1,    19,
      11,    27,    -1,    20,    11,    27,    -1,    21,    11,    27,
      -1,    22,    11,    27,    -1,    23,    11,    27,    -1,    24,
      11,    27,    -1,     5,    12,    27,    -1,    16,    12,    27,
      -1,    17,    12,    27,    -1,    14,    12,    27,    -1,    18,
      12,    27,    -1,    19,    12,    27,    -1,    20,    12,    27,
      -1,    21,    12,    27,    -1,    22,    12,    27,    -1,    23,
      12,    27,    -1,    24,    12,    27,    -1,     5,    13,    27,
      -1,    16,    13,    27,    -1,    17,    13,    27,    -1,    14,
      13,    27,    -1,    18,    13,    27,    -1,    19,    13,    27,
      -1,    20,    13,    27,    -1,    21,    13,    27,    -1,    22,
      13,    27,    -1,    23,    13,    27,    -1,    24,    13,    27,
      -1,     5,    14,    20,    -1,    16,    14,    20,    -1,    17,
      14,    20,    -1,    14,    14,    20,    -1,    18,    14,    20,
      -1,    19,    14,    20,    -1,    20,    14,    20,    -1,    21,
      14,    20,    -1,    22,    14,    20,    -1,    23,    14,    20,
      -1,    24,    14,    20,    -1,     5,    15,    20,    -1,    16,
      15,    20,    -1,    17,    15,    20,    -1,    14,    15,    20,
      -1,    18,    15,    20,    -1,    19,    15,    20,    -1,    20,
      15,    20,    -1,    21,    15,    20,    -1,    22,    15,    20,
      -1,    23,    15,    20,    -1,    24,    15,    20,    -1,     5,
      14,    24,    -1,    16,    14,    24,    -1,    17,    14,    24,
      -1,    14,    14,    24,    -1,    18,    14,    24,    -1,    19,
      14,    24,    -1,    20,    14,    24,    -1,    21,    14,    24,
      -1,    22,    14,    24,    -1,    23,    14,    24,    -1,    24,
      14,    24,    -1,     5,    15,    24,    -1,    16,    15,    24,
      -1,    17,    15,    24,    -1,    14,    15,    24,    -1,    18,
      15,    24,    -1,    19,    15,    24,    -1,    20,    15,    24,
      -1,    21,    15,    24,    -1,    22,    15,    24,    -1,    23,
      15,    24,    -1,    24,    15,    24,    -1,     6,    28,     7,
      -1,    28,    16,    28,    -1,    28,    17,    28,    -1,    23,
       5,    -1,    21,    -1,    22,    -1,    24,     4,    -1,    24,
       3,    -1,    29,    18,    28,    19,    29,    -1,    29,    18,
      28,    -1,    18,    28,    19,    29,    -1,    29,    -1,    28,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   525,   525,   529,   542,   550,   556,   562,   569,   575,
     581,   587,   593,   599,   605,   613,   621,   627,   633,   640,
     646,   652,   659,   665,   671,   677,   686,   694,   700,   706,
     713,   719,   725,   732,   738,   744,   750,   759,   767,   773,
     779,   786,   792,   798,   804,   810,   816,   822,   830,   839,
     846,   853,   861,   868,   875,   882,   889,   896,   903,   913,
     922,   929,   936,   944,   951,   958,   965,   972,   979,   986,
     997,  1002,  1006,  1010,  1015,  1019,  1023,  1028,  1033,  1038,
    1042,  1049,  1054,  1059,  1064,  1070,  1075,  1080,  1085,  1090,
    1095,  1100,  1109,  1114,  1118,  1122,  1127,  1131,  1135,  1139,
    1143,  1147,  1151,  1158,  1163,  1167,  1171,  1176,  1180,  1184,
    1189,  1193,  1197,  1201,  1208,  1211,  1216,  1222,  1230,  1234,
    1237,  1243,  1253,  1258,  1269,  1279,  1282
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NUMBER", "STRING", "PARAM", "LPAR",
  "RPAR", "OP_EQ", "OP_NOTEQ", "OP_GT", "OP_LT", "OP_GTEQ", "OP_LTEQ",
  "OP_IS", "OP_ISNOT", "OP_AND", "OP_OR", "KW_IF", "KW_ELSE",
  "KW_CONNECTED", "KW_CONNECT", "KW_IGNORE", "KW_COPY", "KW_SET",
  "UNRECOGNIZED_TOKEN", "$accept", "value", "expr", "action", "statement", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    26,    27,    27,    28,    28,    28,    28,    28,    28,
      28,    28,    28,    28,    28,    28,    28,    28,    28,    28,
      28,    28,    28,    28,    28,    28,    28,    28,    28,    28,
      28,    28,    28,    28,    28,    28,    28,    28,    28,    28,
      28,    28,    28,    28,    28,    28,    28,    28,    28,    28,
      28,    28,    28,    28,    28,    28,    28,    28,    28,    28,
      28,    28,    28,    28,    28,    28,    28,    28,    28,    28,
      28,    28,    28,    28,    28,    28,    28,    28,    28,    28,
      28,    28,    28,    28,    28,    28,    28,    28,    28,    28,
      28,    28,    28,    28,    28,    28,    28,    28,    28,    28,
      28,    28,    28,    28,    28,    28,    28,    28,    28,    28,
      28,    28,    28,    28,    28,    28,    28,    29,    29,    29,
      29,    29,    30,    30,    30,    30,    30
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     1,     1,
       2,     2,     5,     3,     4,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,   118,
     119,     0,     0,   126,   125,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   117,     0,     0,     0,     0,
       0,     0,     0,     0,   121,   120,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     1,     3,     2,
       4,    15,    26,    37,    48,    59,    70,    92,    81,   103,
       0,   114,     7,    18,    29,    40,    51,    62,    73,    95,
      84,   106,     5,    16,    27,    38,    49,    60,    71,    93,
      82,   104,     6,    17,    28,    39,    50,    61,    72,    94,
      83,   105,     8,    19,    30,    41,    52,    63,    74,    96,
      85,   107,     0,     9,    20,    31,    42,    53,    64,    75,
      97,    86,   108,    10,    21,    32,    43,    54,    65,    76,
      98,    87,   109,    11,    22,    33,    44,    55,    66,    77,
      99,    88,   110,    12,    23,    34,    45,    56,    67,    78,
     100,    89,   111,    13,    24,    35,    46,    57,    68,    79,
     101,    90,   112,    14,    25,    36,    47,    58,    69,    80,
     102,    91,   113,   115,   116,   123,   118,   119,     0,     0,
     124,     0,   122
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,   120,    13,    14,    15
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -161
static const yytype_int16 yypact[] =
{
      86,   184,   130,   192,   200,   208,   110,   216,   224,   232,
     240,   176,   165,   -14,    14,    16,    11,    11,    11,    11,
      11,    11,    -7,   117,   248,   232,   240,   256,   264,    -6,
      11,    11,    11,    11,    11,    11,   118,   119,    11,    11,
      11,    11,    11,    11,   146,   260,    11,    11,    11,    11,
      11,    11,   261,   262,    11,    11,    11,    11,    11,    11,
     147,   263,   305,    11,    11,    11,    11,    11,    11,   268,
     269,    11,    11,    11,    11,    11,    11,   270,   271,    11,
      11,    11,    11,    11,    11,   276,   277,    11,    11,    11,
      11,    11,    11,   278,   279,  -161,    11,    11,    11,    11,
      11,    11,   284,   285,  -161,  -161,    11,    11,    11,    11,
      11,    11,   286,   287,   130,   130,   130,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
     292,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,   296,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,  -161,
    -161,  -161,  -161,  -161,    17,   309,  -161,  -161,    35,    21,
    -161,   296,  -161
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -161,   -12,    -2,  -160,  -161
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      29,   131,   114,   115,    62,   121,   122,   123,   124,   125,
     114,   115,   240,   126,   118,   119,   117,   127,   132,   133,
     134,   135,   136,   137,   104,   105,   142,   143,   144,   145,
     146,   147,   116,   114,   152,   153,   154,   155,   156,   157,
      95,     0,   162,   163,   164,   165,   166,   167,     0,     0,
       0,   173,   174,   175,   176,   177,   178,     0,     0,   183,
     184,   185,   186,   187,   188,     0,     0,   193,   194,   195,
     196,   197,   198,     0,     0,   203,   204,   205,   206,   207,
     208,   242,     0,     0,   213,   214,   215,   216,   217,   218,
       0,     1,     2,     0,   223,   224,   225,   226,   227,   228,
       3,     0,     4,     5,     6,     7,     8,     9,    10,    11,
      12,     0,   233,   234,   235,     1,     2,     0,    54,    55,
      56,    57,    58,    59,    60,    61,     4,     5,    24,     7,
       8,    25,    26,    27,    28,     1,     2,   128,   138,   140,
       0,   129,   139,   141,     3,     0,     4,     5,    24,     7,
       8,    25,    26,    27,    28,    30,    31,    32,    33,    34,
      35,    36,    37,     0,     0,     0,   148,   168,   104,   105,
     149,   169,     0,   106,   107,   108,   109,   110,   111,   112,
     113,    95,     0,     0,    96,    97,    98,    99,   100,   101,
     102,   103,    16,    17,    18,    19,    20,    21,    22,    23,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    54,    55,    56,    57,
      58,    59,   130,    61,    96,    97,    98,    99,   100,   101,
     102,   103,   106,   107,   108,   109,   110,   111,   112,   113,
     150,   158,   160,   170,   151,   159,   161,   171,   179,   181,
     189,   191,   180,   182,   190,   192,   199,   201,   209,   211,
     200,   202,   210,   212,   219,   221,   229,   231,   220,   222,
     230,   232,   168,     0,     0,     0,   169,   236,   237,   238,
     239,   114,   115,     0,   172,   114,   115,     0,   241
};

static const yytype_int16 yycheck[] =
{
       2,     7,    16,    17,     6,    17,    18,    19,    20,    21,
      16,    17,   172,    20,     3,     4,     0,    24,    30,    31,
      32,    33,    34,    35,     3,     4,    38,    39,    40,    41,
      42,    43,    18,    16,    46,    47,    48,    49,    50,    51,
       5,    -1,    54,    55,    56,    57,    58,    59,    -1,    -1,
      -1,    63,    64,    65,    66,    67,    68,    -1,    -1,    71,
      72,    73,    74,    75,    76,    -1,    -1,    79,    80,    81,
      82,    83,    84,    -1,    -1,    87,    88,    89,    90,    91,
      92,   241,    -1,    -1,    96,    97,    98,    99,   100,   101,
      -1,     5,     6,    -1,   106,   107,   108,   109,   110,   111,
      14,    -1,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    -1,   114,   115,   116,     5,     6,    -1,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,     5,     6,    20,    20,    20,
      -1,    24,    24,    24,    14,    -1,    16,    17,    18,    19,
      20,    21,    22,    23,    24,     8,     9,    10,    11,    12,
      13,    14,    15,    -1,    -1,    -1,    20,    20,     3,     4,
      24,    24,    -1,     8,     9,    10,    11,    12,    13,    14,
      15,     5,    -1,    -1,     8,     9,    10,    11,    12,    13,
      14,    15,     8,     9,    10,    11,    12,    13,    14,    15,
       8,     9,    10,    11,    12,    13,    14,    15,     8,     9,
      10,    11,    12,    13,    14,    15,     8,     9,    10,    11,
      12,    13,    14,    15,     8,     9,    10,    11,    12,    13,
      14,    15,     8,     9,    10,    11,    12,    13,    14,    15,
       8,     9,    10,    11,    12,    13,    14,    15,     8,     9,
      10,    11,    12,    13,    14,    15,     8,     9,    10,    11,
      12,    13,    14,    15,     8,     9,    10,    11,    12,    13,
      14,    15,     8,     9,    10,    11,    12,    13,    14,    15,
      20,    20,    20,    20,    24,    24,    24,    24,    20,    20,
      20,    20,    24,    24,    24,    24,    20,    20,    20,    20,
      24,    24,    24,    24,    20,    20,    20,    20,    24,    24,
      24,    24,    20,    -1,    -1,    -1,    24,    21,    22,    23,
      24,    16,    17,    -1,    19,    16,    17,    -1,    19
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     5,     6,    14,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    28,    29,    30,     8,     9,    10,    11,
      12,    13,    14,    15,    18,    21,    22,    23,    24,    28,
       8,     9,    10,    11,    12,    13,    14,    15,     8,     9,
      10,    11,    12,    13,    14,    15,     8,     9,    10,    11,
      12,    13,    14,    15,     8,     9,    10,    11,    12,    13,
      14,    15,    28,     8,     9,    10,    11,    12,    13,    14,
      15,     8,     9,    10,    11,    12,    13,    14,    15,     8,
       9,    10,    11,    12,    13,    14,    15,     8,     9,    10,
      11,    12,    13,    14,    15,     5,     8,     9,    10,    11,
      12,    13,    14,    15,     3,     4,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,     0,     3,     4,
      27,    27,    27,    27,    27,    27,    20,    24,    20,    24,
      14,     7,    27,    27,    27,    27,    27,    27,    20,    24,
      20,    24,    27,    27,    27,    27,    27,    27,    20,    24,
      20,    24,    27,    27,    27,    27,    27,    27,    20,    24,
      20,    24,    27,    27,    27,    27,    27,    27,    20,    24,
      20,    24,    19,    27,    27,    27,    27,    27,    27,    20,
      24,    20,    24,    27,    27,    27,    27,    27,    27,    20,
      24,    20,    24,    27,    27,    27,    27,    27,    27,    20,
      24,    20,    24,    27,    27,    27,    27,    27,    27,    20,
      24,    20,    24,    27,    27,    27,    27,    27,    27,    20,
      24,    20,    24,    27,    27,    27,    27,    27,    27,    20,
      24,    20,    24,    28,    28,    28,    21,    22,    23,    24,
      29,    19,    29
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (&yylloc, data, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, &yylloc, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, &yylloc, MACRO_YYSCANNER)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location, data); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, _VSCGParserData * data)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, data)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    _VSCGParserData * data;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
  YYUSE (data);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, _VSCGParserData * data)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, data)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    _VSCGParserData * data;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, data);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, _VSCGParserData * data)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, data)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    _VSCGParserData * data;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , data);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule, data); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
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



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, _VSCGParserData * data)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, data)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    _VSCGParserData * data;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (data);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {
      case 3: /* "NUMBER" */

/* Line 1000 of yacc.c  */
#line 464 "hdPrman/virtualStructConditionalGrammar.yy"
	{
  if ((yyvaluep->string))
  {
      free((yyvaluep->string));
  }
  (yyvaluep->string) = 0;
};

/* Line 1000 of yacc.c  */
#line 1763 "hdPrman/virtualStructConditionalGrammar.tab.cpp"
	break;
      case 4: /* "STRING" */

/* Line 1000 of yacc.c  */
#line 464 "hdPrman/virtualStructConditionalGrammar.yy"
	{
  if ((yyvaluep->string))
  {
      free((yyvaluep->string));
  }
  (yyvaluep->string) = 0;
};

/* Line 1000 of yacc.c  */
#line 1778 "hdPrman/virtualStructConditionalGrammar.tab.cpp"
	break;
      case 5: /* "PARAM" */

/* Line 1000 of yacc.c  */
#line 464 "hdPrman/virtualStructConditionalGrammar.yy"
	{
  if ((yyvaluep->string))
  {
      free((yyvaluep->string));
  }
  (yyvaluep->string) = 0;
};

/* Line 1000 of yacc.c  */
#line 1793 "hdPrman/virtualStructConditionalGrammar.tab.cpp"
	break;
      case 27: /* "value" */

/* Line 1000 of yacc.c  */
#line 480 "hdPrman/virtualStructConditionalGrammar.yy"
	{
  if ((yyvaluep->value))
  {
      delete (yyvaluep->value);
  }
  (yyvaluep->value) = 0;
};

/* Line 1000 of yacc.c  */
#line 1808 "hdPrman/virtualStructConditionalGrammar.tab.cpp"
	break;
      case 29: /* "action" */

/* Line 1000 of yacc.c  */
#line 472 "hdPrman/virtualStructConditionalGrammar.yy"
	{
  if ((yyvaluep->action))
  {
      delete (yyvaluep->action);
  }
  (yyvaluep->action) = 0;
};

/* Line 1000 of yacc.c  */
#line 1823 "hdPrman/virtualStructConditionalGrammar.tab.cpp"
	break;

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (_VSCGParserData * data);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (_VSCGParserData * data)
#else
int
yyparse (data)
    _VSCGParserData * data;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Location data for the lookahead symbol.  */
YYLTYPE yylloc;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.
       `yyls': related to locations.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[2];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;

#if YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 1;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);

	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
	YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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
      if (yyn == 0 || yyn == YYTABLE_NINF)
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
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
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1455 of yacc.c  */
#line 525 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    (yyval.value) = new _VSCGValueContainer(VtValue((yyvsp[(1) - (1)].string)));
    free((yyvsp[(1) - (1)].string));
;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 529 "hdPrman/virtualStructConditionalGrammar.yy"
    {    
    (yyval.value) = new _VSCGValueContainer(VtValue(atof((yyvsp[(1) - (1)].string))));
    free((yyvsp[(1) - (1)].string));
;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 542 "hdPrman/virtualStructConditionalGrammar.yy"
    {

    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken((yyvsp[(1) - (3)].string)), (yyvsp[(3) - (3)].value)->value));

    free((yyvsp[(1) - (3)].string));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 550 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'and', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("and"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 556 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'or', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("or"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 562 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'is', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("is"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 569 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'if', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("if"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 575 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'else', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("else"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 581 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connected', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("connected"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 587 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connect', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("connect"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 593 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'ignore', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("ignore"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 599 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'copy', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("copy"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 605 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'set', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("set"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 613 "hdPrman/virtualStructConditionalGrammar.yy"
    {

    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken((yyvsp[(1) - (3)].string)), (yyvsp[(3) - (3)].value)->value));

    free((yyvsp[(1) - (3)].string));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 621 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'and', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("and"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 627 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'or', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("or"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 633 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'is', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("is"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 640 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'if', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("if"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 646 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'else', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("else"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 652 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connected', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(
                    TfToken("connected"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 659 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connect', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("connect"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 665 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'ignore', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("ignore"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 671 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'copy', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("copy"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 677 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'set', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("set"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 686 "hdPrman/virtualStructConditionalGrammar.yy"
    {

    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken((yyvsp[(1) - (3)].string)), (yyvsp[(3) - (3)].value)->value));

    free((yyvsp[(1) - (3)].string));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 694 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'and', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("and"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 700 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'or', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("or"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 706 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'is', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("is"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 713 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'if', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("if"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 719 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'else', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("else"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 725 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connected', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThan(
                    TfToken("connected"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 732 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connect', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("connect"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 738 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'ignore', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("ignore"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 744 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'copy', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("copy"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 750 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'set', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("set"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 759 "hdPrman/virtualStructConditionalGrammar.yy"
    {

    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken((yyvsp[(1) - (3)].string)), (yyvsp[(3) - (3)].value)->value));

    free((yyvsp[(1) - (3)].string));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 767 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'and', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("and"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 773 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'or', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("or"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 779 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'is', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("is"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 786 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'if', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("if"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 792 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'else', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("else"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 798 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connected', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("connected"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 804 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connect', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("connect"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 810 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'ignore', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("ignore"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 816 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'copy', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("copy"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 822 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'set', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("set"), (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 830 "hdPrman/virtualStructConditionalGrammar.yy"
    {

    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken((yyvsp[(1) - (3)].string)),
                    (yyvsp[(3) - (3)].value)->value));

    free((yyvsp[(1) - (3)].string));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 839 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'and', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("and"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 846 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'or', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("or"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 853 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'is', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("is"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 861 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'if', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("if"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 868 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'else', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("else"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 875 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connected', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("connected"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 882 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connect', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("connect"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 889 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'ignore', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("ignore"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 896 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'copy', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("copy"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 903 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'set', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("set"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 913 "hdPrman/virtualStructConditionalGrammar.yy"
    {

    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken((yyvsp[(1) - (3)].string)),
                    (yyvsp[(3) - (3)].value)->value));

    free((yyvsp[(1) - (3)].string));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 922 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'and', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("and"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 929 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'or', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("or"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 936 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'is', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("is"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 944 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'if', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("if"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 951 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'else', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("else"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 958 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connected', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("connected"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 965 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connect', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("connect"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 972 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'ignore', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("ignore"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 979 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'copy', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("copy"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 986 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'set', special case */
    (yyval.condition) = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("set"),
                    (yyvsp[(3) - (3)].value)->value));
    delete (yyvsp[(3) - (3)].value);
;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 997 "hdPrman/virtualStructConditionalGrammar.yy"
    {

    (yyval.condition) = data->NewCondition(new ConditionalParamIsConnected(TfToken((yyvsp[(1) - (3)].string))));
    free((yyvsp[(1) - (3)].string));
;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 1002 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'and', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsConnected(TfToken("and")));
;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1006 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'or', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsConnected(TfToken("or")));
;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 1010 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'is', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsConnected(TfToken("is")));
;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1015 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'if', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsConnected(TfToken("if")));
;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1019 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'else', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsConnected(TfToken("else")));
;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1023 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connected', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsConnected(
            TfToken("connected")));
;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1028 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connect', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsConnected(
            TfToken("connect")));
;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1033 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'ignore', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsConnected(
            TfToken("ignore")));
;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1038 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'copy', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsConnected(TfToken("copy")));
;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 1042 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'set', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsConnected(TfToken("set")));
;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 1049 "hdPrman/virtualStructConditionalGrammar.yy"
    {

    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotConnected(TfToken((yyvsp[(1) - (3)].string))));
    free((yyvsp[(1) - (3)].string));
;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 1054 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'and', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("and")));
;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1059 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'or', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("or")));
;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1064 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'is', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("is")));
;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 1070 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'if', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("if")));
;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 1075 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'else', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("else")));
;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 1080 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connected', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("connected")));
;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 1085 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connect', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("connect")));
;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 1090 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'ignore', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("ignore")));
;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 1095 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'copy', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("copy")));
;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 1100 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'set', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("set")));
;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 1109 "hdPrman/virtualStructConditionalGrammar.yy"
    {

    (yyval.condition) = data->NewCondition(new ConditionalParamIsSet(TfToken((yyvsp[(1) - (3)].string))));
    free((yyvsp[(1) - (3)].string));
;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 1114 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'and', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsSet(TfToken("and")));
;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1118 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'or', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsSet(TfToken("or")));
;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 1122 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'is', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsSet(TfToken("is")));
;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1127 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'if', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsSet(TfToken("if")));
;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1131 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'else', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsSet(TfToken("else")));
;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1135 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connected', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsSet(TfToken("connected")));
;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1139 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connect', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsSet(TfToken("connect")));
;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1143 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'ignore', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsSet(TfToken("ignore")));
;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1147 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'copy', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsSet(TfToken("copy")));
;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1151 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'set', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsSet(TfToken("set")));
;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1158 "hdPrman/virtualStructConditionalGrammar.yy"
    {

    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotSet(TfToken((yyvsp[(1) - (3)].string))));
    free((yyvsp[(1) - (3)].string));
;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1163 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'and', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotSet(TfToken("and")));
;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1167 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'or', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotSet(TfToken("or")));
;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 1171 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'is', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotSet(TfToken("is")));
;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 1176 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'if', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotSet(TfToken("if")));
;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 1180 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'else', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotSet(TfToken("else")));
;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1184 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connected', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotSet(
            TfToken("connected")));
;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 1189 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    /* PARAM named 'connect', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotSet(TfToken("connect")));
;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 1193 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'ignore', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotSet(TfToken("ignore")));
;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 1197 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'copy', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotSet(TfToken("copy")));
;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 1201 "hdPrman/virtualStructConditionalGrammar.yy"
    {
        /* PARAM named 'set', special case */
    (yyval.condition) = data->NewCondition(new ConditionalParamIsNotSet(TfToken("set")));
;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 1208 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    (yyval.condition) = (yyvsp[(2) - (3)].condition);
 ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1211 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    (yyval.condition) = data->NewCondition(new ConditionalAnd(
            data->ClaimCondition((yyvsp[(1) - (3)].condition)),
            data->ClaimCondition((yyvsp[(3) - (3)].condition))));
 ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1216 "hdPrman/virtualStructConditionalGrammar.yy"
    {

    (yyval.condition) = data->NewCondition(new ConditionalOr(
            data->ClaimCondition((yyvsp[(1) - (3)].condition)),
            data->ClaimCondition((yyvsp[(3) - (3)].condition)))); 
 ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 1222 "hdPrman/virtualStructConditionalGrammar.yy"
    {

    TfToken paramName((yyvsp[(2) - (2)].string));
    free((yyvsp[(2) - (2)].string));

    (yyval.action) = new _VSCGAction(_VSCGAction::CopyParam, VtValue(paramName));

;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1230 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    (yyval.action) = new _VSCGAction(_VSCGAction::Connect);
   
;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1234 "hdPrman/virtualStructConditionalGrammar.yy"
    {
   (yyval.action) = new _VSCGAction(_VSCGAction::Ignore);
;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1237 "hdPrman/virtualStructConditionalGrammar.yy"
    {

    std::string value((yyvsp[(2) - (2)].string));
    free((yyvsp[(2) - (2)].string));
    (yyval.action) = new _VSCGAction(_VSCGAction::SetConstant, VtValue(value));
;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1243 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    (yyval.action) = new _VSCGAction(_VSCGAction::SetConstant, VtValue(atof((yyvsp[(2) - (2)].string))));
   free((yyvsp[(2) - (2)].string));
;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1253 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    data->action = (yyvsp[(1) - (5)].action);
    data->rootCondition = data->ClaimCondition((yyvsp[(3) - (5)].condition));
    data->fallbackAction = (yyvsp[(5) - (5)].action);
 ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1258 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    data->action = (yyvsp[(1) - (3)].action);
    data->rootCondition = data->ClaimCondition((yyvsp[(3) - (3)].condition));

    if ((yyvsp[(1) - (3)].action)->action == _VSCGAction::Ignore) {
        data->fallbackAction = new _VSCGAction(_VSCGAction::Connect);
    } else {
        data->fallbackAction = new _VSCGAction(_VSCGAction::Ignore);
    }
 
 ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1269 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    
    data->action = new _VSCGAction(_VSCGAction::Connect);

    data->rootCondition = data->ClaimCondition((yyvsp[(2) - (4)].condition));


    data->fallbackAction = (yyvsp[(4) - (4)].action);
    
 ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1279 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    data->action = (yyvsp[(1) - (1)].action);
 ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1282 "hdPrman/virtualStructConditionalGrammar.yy"
    {
    data->action = new _VSCGAction(_VSCGAction::Connect);
    data->rootCondition = data->ClaimCondition((yyvsp[(1) - (1)].condition));
    data->fallbackAction = new _VSCGAction(_VSCGAction::Ignore);
 ;}
    break;



/* Line 1455 of yacc.c  */
#line 3611 "hdPrman/virtualStructConditionalGrammar.tab.cpp"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, data, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (&yylloc, data, yymsg);
	  }
	else
	  {
	    yyerror (&yylloc, data, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

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
		      yytoken, &yylval, &yylloc, data);
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp, data);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, data, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc, data);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp, data);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 1289 "hdPrman/virtualStructConditionalGrammar.yy"



 

/* Called by yyparse on error */
void virtualStructConditionalGrammarYyerror(
        YYLTYPE *llocp, _VSCGParserData * data, const char *s)
{
    data->parseError = s;
}

typedef struct yy_buffer_state * YY_BUFFER_STATE;
YY_BUFFER_STATE virtualStructConditionalGrammarYy_scan_string(
        const char *, void * );
void virtualStructConditionalGrammarYy_delete_buffer(
        YY_BUFFER_STATE, void * );
int virtualStructConditionalGrammarYylex_init (void ** yyscanner);
int virtualStructConditionalGrammarYylex_destroy (void * yyscanner);



//----------------------------------------------------------------------------

MatfiltVstructConditionalEvaluator::Ptr
MatfiltVstructConditionalEvaluator::Parse(const std::string & inputExpr)
{
    _VSCGParserData data;
    if (virtualStructConditionalGrammarYylex_init(&data.yyscanner)) {
        TF_CODING_ERROR("_VSCGParser: error initializing scanner");
        return nullptr;
    }
    YY_BUFFER_STATE bufferstate =
        virtualStructConditionalGrammarYy_scan_string(
                inputExpr.c_str(), data.yyscanner);
    MatfiltVstructConditionalEvaluator::Ptr result(
            new MatfiltVstructConditionalEvaluator);
    if (virtualStructConditionalGrammarYyparse(&data) == 0) {
        //hintsAttr = data.getHints(prefix, secondaryPrefix);
        
        result->_impl = new MatfiltVstructConditionalEvaluatorImpl;

        result->_impl->condition = data.rootCondition;
        result->_impl->action = data.action;
        result->_impl->fallbackAction = data.fallbackAction;

        data.rootCondition = nullptr;
        data.action = nullptr;
        data.fallbackAction = nullptr;
    } else {
        TF_CODING_ERROR("_VSCGParser: Error parsing '%s'", inputExpr.c_str());
    }
    if (!data.parseError.empty()) {
        TF_CODING_ERROR("_VSCGParser: Error parsing '%s': %s",
                inputExpr.c_str(),
                data.parseError.c_str());
    }

    virtualStructConditionalGrammarYy_delete_buffer(
            bufferstate, data.yyscanner);
    virtualStructConditionalGrammarYylex_destroy(data.yyscanner);

    // TODO
    return result;
}

void MatfiltVstructConditionalEvaluator::Evaluate(
        const SdfPath & nodeId,
        const TfToken & nodeInput,
        const SdfPath & upstreamNodeId,
        const TfToken & upstreamNodeOutput,
        const NdrTokenVec & shaderTypePriority,
        MatfiltNetwork & network) const
{
    if (!_impl) {
        TF_CODING_ERROR("MatfiltVstructConditionalEvaluator: No impl");
        return;
    }

    // Find node
    auto I = network.nodes.find(nodeId);
    if (I == network.nodes.end()) {
        TF_CODING_ERROR("MatfiltVstructConditionalEvaluator: Cannot eval "
                "for node %s; not found in network",
                nodeId.GetText());
        return;
    }
    MatfiltNode & node = (*I).second;

    // if it's already connected explicitly, don't do anything
    if (node.inputConnections.find(nodeInput) != node.inputConnections.end()) {
        return;
    }

    _VSCGAction * chosenAction = nullptr;

    // Get upstream node.
    auto I2 = network.nodes.find(upstreamNodeId);
    if (I2 == network.nodes.end()) {
        // No upstream node; silently ignore
        return;
    }
    const MatfiltNode & upstreamNode = (*I2).second;

    // Decide action to perform.
    if (_impl->condition) {
        if (_impl->condition->Eval(upstreamNode, shaderTypePriority)) {
            chosenAction = _impl->action;
        } else {
            chosenAction = _impl->fallbackAction;
        }            
    } else {
        chosenAction = _impl->action;
    }
    if (!chosenAction) {
        TF_CODING_ERROR("MatfiltVstructConditionalEvaluator: NULL action");
        return;
    }

    // Execute action.
    switch (chosenAction->action) {
    case _VSCGAction::Ignore:
        break;
    case _VSCGAction::Connect:
        node.inputConnections[nodeInput] =
                {{upstreamNodeId, upstreamNodeOutput}};
        break;
    case _VSCGAction::SetConstant:
    {
        // convert the constant to the expected type    
        auto& reg = SdrRegistry::GetInstance();
        SdrShaderNodeConstPtr sdrNode = 
                reg.GetShaderNodeByIdentifier(
                        node.nodeTypeId, shaderTypePriority);
        if (!sdrNode) {
            // TODO, warn
            break;
        }
        NdrPropertyConstPtr ndrProp = sdrNode->GetInput(nodeInput);
        if (!ndrProp) {
            // TODO, warn
            break;
        }
        VtValue value = chosenAction->value;
        TfToken inputType = ndrProp->GetType();
        if (value.IsHolding<std::string>()) {
            if (inputType == SdrPropertyTypes->String) {
                node.parameters[nodeInput] = value;
            } else {
                TF_CODING_ERROR("MatfiltVstructConditionalEvaluator: "
                        "Expected string but found %s\n",
                        inputType.GetText());
                break;
            }
        } else if (value.IsHolding<double>()) {
            // parser always stores numbers as double.
            double doubleValue = value.UncheckedGet<double>();
            VtValue resultValue;
            if (inputType == SdrPropertyTypes->Int) {
                resultValue = VtValue(static_cast<int>(doubleValue));
            } else if (inputType == SdrPropertyTypes->Float) {
                resultValue = VtValue(static_cast<float>(doubleValue));
            }
            if (!resultValue.IsEmpty()) {
                node.parameters[nodeInput] = resultValue;
            } else {
                TF_CODING_ERROR("MatfiltVstructConditionalEvaluator: "
                        "Empty result");
            }
            break;
        } else {
            TF_CODING_ERROR("MatfiltVstructConditionalEvaluator: "
                    "Unhandled type %s\n",
                    value.GetTypeName().c_str());
            break;
        }
        break;
    }
    case _VSCGAction::CopyParam:
    {
        if (chosenAction->value.IsHolding<TfToken>()) {
            const TfToken & copyParamName =
                    chosenAction->value.UncheckedGet<TfToken>();
            // confirm that parameter types are equivalent
            // via Sdr                            
            auto& reg = SdrRegistry::GetInstance();
            SdrShaderNodeConstPtr sdrNode = 
                    reg.GetShaderNodeByIdentifier(
                            node.nodeTypeId, shaderTypePriority);
            SdrShaderNodeConstPtr sdrUpstreamNode = 
                    reg.GetShaderNodeByIdentifier(
                            upstreamNode.nodeTypeId,
                            shaderTypePriority);
            if (!sdrNode || !sdrUpstreamNode) {
                // TODO warn?
                break;
            }
            NdrPropertyConstPtr ndrProp =
                    sdrNode->GetInput(nodeInput);

            NdrPropertyConstPtr ndrUpstreamProp =
                    sdrUpstreamNode->GetInput(copyParamName);

            if (!ndrProp || !ndrUpstreamProp) {
                // TODO warn?
                break;
            }

            // TODO, convert between int and float
            if (ndrProp->GetType() == ndrUpstreamProp->GetType()) {
                auto I = upstreamNode.parameters.find(copyParamName);

                if (I != upstreamNode.parameters.end()) {
                    // authored value
                    node.parameters[nodeInput] = (*I).second;
                } else {
                    // use default
                    node.parameters[nodeInput] =
                            ndrUpstreamProp->GetDefaultValue();
                }
            } else {
                // TODO warn?
            }
        }
        break;
    }
    }
    // TODO
}

MatfiltVstructConditionalEvaluator::~MatfiltVstructConditionalEvaluator() {
    delete _impl;
}

