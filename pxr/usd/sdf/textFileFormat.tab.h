//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
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
     TOK_BEZIER = 271,
     TOK_CLASS = 272,
     TOK_CONFIG = 273,
     TOK_CONNECT = 274,
     TOK_CURVE = 275,
     TOK_CUSTOM = 276,
     TOK_CUSTOMDATA = 277,
     TOK_DEF = 278,
     TOK_DEFAULT = 279,
     TOK_DELETE = 280,
     TOK_DICTIONARY = 281,
     TOK_DISPLAYUNIT = 282,
     TOK_DOC = 283,
     TOK_HELD = 284,
     TOK_HERMITE = 285,
     TOK_INHERITS = 286,
     TOK_KIND = 287,
     TOK_LINEAR = 288,
     TOK_LOOP = 289,
     TOK_NAMECHILDREN = 290,
     TOK_NONE = 291,
     TOK_NONE_LC = 292,
     TOK_OFFSET = 293,
     TOK_OSCILLATE = 294,
     TOK_OVER = 295,
     TOK_PERMISSION = 296,
     TOK_POST = 297,
     TOK_PRE = 298,
     TOK_PAYLOAD = 299,
     TOK_PREFIX_SUBSTITUTIONS = 300,
     TOK_SUFFIX_SUBSTITUTIONS = 301,
     TOK_PREPEND = 302,
     TOK_PROPERTIES = 303,
     TOK_REFERENCES = 304,
     TOK_RELOCATES = 305,
     TOK_REL = 306,
     TOK_RENAMES = 307,
     TOK_REORDER = 308,
     TOK_ROOTPRIMS = 309,
     TOK_REPEAT = 310,
     TOK_RESET = 311,
     TOK_SCALE = 312,
     TOK_SLOPED = 313,
     TOK_SPECIALIZES = 314,
     TOK_SPLINE = 315,
     TOK_SUBLAYERS = 316,
     TOK_SYMMETRYARGUMENTS = 317,
     TOK_SYMMETRYFUNCTION = 318,
     TOK_TIME_SAMPLES = 319,
     TOK_UNIFORM = 320,
     TOK_VARIANTS = 321,
     TOK_VARIANTSET = 322,
     TOK_VARIANTSETS = 323,
     TOK_VARYING = 324
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif




