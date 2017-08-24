//
// Copyright 2016 Pixar
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
     TOK_ATTRIBUTES = 271,
     TOK_CLASS = 272,
     TOK_CONFIG = 273,
     TOK_CONNECT = 274,
     TOK_CUSTOM = 275,
     TOK_CUSTOMDATA = 276,
     TOK_DEF = 277,
     TOK_DEFAULT = 278,
     TOK_DELETE = 279,
     TOK_DICTIONARY = 280,
     TOK_DISPLAYUNIT = 281,
     TOK_DOC = 282,
     TOK_INHERITS = 283,
     TOK_KIND = 284,
     TOK_MAPPER = 285,
     TOK_NAMECHILDREN = 286,
     TOK_NONE = 287,
     TOK_OFFSET = 288,
     TOK_OVER = 289,
     TOK_PERMISSION = 290,
     TOK_PAYLOAD = 291,
     TOK_PREFIX_SUBSTITUTIONS = 292,
     TOK_SUFFIX_SUBSTITUTIONS = 293,
     TOK_PREPEND = 294,
     TOK_PROPERTIES = 295,
     TOK_REFERENCES = 296,
     TOK_RELOCATES = 297,
     TOK_REL = 298,
     TOK_RENAMES = 299,
     TOK_REORDER = 300,
     TOK_ROOTPRIMS = 301,
     TOK_SCALE = 302,
     TOK_SPECIALIZES = 303,
     TOK_SUBLAYERS = 304,
     TOK_SYMMETRYARGUMENTS = 305,
     TOK_SYMMETRYFUNCTION = 306,
     TOK_TIME_SAMPLES = 307,
     TOK_UNIFORM = 308,
     TOK_VARIANTS = 309,
     TOK_VARIANTSET = 310,
     TOK_VARIANTSETS = 311,
     TOK_VARYING = 312
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif




