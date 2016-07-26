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
     TOK_ATTRIBUTES = 270,
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
     TOK_MAPPER = 284,
     TOK_NAMECHILDREN = 285,
     TOK_NONE = 286,
     TOK_OFFSET = 287,
     TOK_OVER = 288,
     TOK_PERMISSION = 289,
     TOK_PAYLOAD = 290,
     TOK_PREFIX_SUBSTITUTIONS = 291,
     TOK_PROPERTIES = 292,
     TOK_REFERENCES = 293,
     TOK_RELOCATES = 294,
     TOK_REL = 295,
     TOK_RENAMES = 296,
     TOK_REORDER = 297,
     TOK_ROOTPRIMS = 298,
     TOK_SCALE = 299,
     TOK_SPECIALIZES = 300,
     TOK_SUBLAYERS = 301,
     TOK_SYMMETRYARGUMENTS = 302,
     TOK_SYMMETRYFUNCTION = 303,
     TOK_TIME_SAMPLES = 304,
     TOK_UNIFORM = 305,
     TOK_VARIANTS = 306,
     TOK_VARIANTSET = 307,
     TOK_VARIANTSETS = 308,
     TOK_VARYING = 309
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif




