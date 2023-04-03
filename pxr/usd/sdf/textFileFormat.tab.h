/* A Bison parser, made by GNU Bison 2.7.12-4996.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.
   
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

#ifndef YY_TEXTFILEFORMATYY_TEXTFILEFORMAT_TAB_H_INCLUDED
# define YY_TEXTFILEFORMATYY_TEXTFILEFORMAT_TAB_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int textFileFormatYydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOK_NL = 258,
     TOK_MAGIC = 259,
     TOK_SYNTAX_ERROR = 260,
     TOK_ASSETREF = 261,
     TOK_PATHREF = 262,
     TOK_IDENTIFIER = 263,
     TOK_CXX_NAMESPACED_IDENTIFIER = 264,
     TOK_NAMESPACED_IDENTIFIER = 265,
     TOK_NUMBER = 266,
     TOK_STRING = 267,
     TOK_ABSTRACT = 268,
     TOK_ADD = 269,
     TOK_APPEND = 270,
     TOK_CLASS = 271,
     TOK_CONFIG = 272,
     TOK_CONNECT = 273,
     TOK_CUSTOM = 274,
     TOK_CUSTOMDATA = 275,
     TOK_DEF = 276,
     TOK_DEFAULT = 277,
     TOK_DELETE = 278,
     TOK_DICTIONARY = 279,
     TOK_DISPLAYUNIT = 280,
     TOK_DOC = 281,
     TOK_INHERITS = 282,
     TOK_KIND = 283,
     TOK_NAMECHILDREN = 284,
     TOK_NONE = 285,
     TOK_OFFSET = 286,
     TOK_OVER = 287,
     TOK_PERMISSION = 288,
     TOK_PAYLOAD = 289,
     TOK_PREFIX_SUBSTITUTIONS = 290,
     TOK_SUFFIX_SUBSTITUTIONS = 291,
     TOK_PREPEND = 292,
     TOK_PROPERTIES = 293,
     TOK_REFERENCES = 294,
     TOK_RELOCATES = 295,
     TOK_REL = 296,
     TOK_RENAMES = 297,
     TOK_REORDER = 298,
     TOK_ROOTPRIMS = 299,
     TOK_SCALE = 300,
     TOK_SPECIALIZES = 301,
     TOK_SUBLAYERS = 302,
     TOK_SYMMETRYARGUMENTS = 303,
     TOK_SYMMETRYFUNCTION = 304,
     TOK_TIME_SAMPLES = 305,
     TOK_UNIFORM = 306,
     TOK_VARIANTS = 307,
     TOK_VARIANTSET = 308,
     TOK_VARIANTSETS = 309,
     TOK_VARYING = 310
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int textFileFormatYyparse (void *YYPARSE_PARAM);
#else
int textFileFormatYyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int textFileFormatYyparse (PXR_NS::Sdf_TextParserContext *context);
#else
int textFileFormatYyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_TEXTFILEFORMATYY_TEXTFILEFORMAT_TAB_H_INCLUDED  */
