/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include "OgreStableHeaders.h"
#include "OgreScriptLexer.h"

namespace Ogre {
    ScriptTokenList ScriptLexer::tokenize(String str, const String& source)
    {
        String error;
        ScriptTokenList ret = _tokenize(str, source.c_str(), error);

        if (!error.empty())
            LogManager::getSingleton().logError("ScriptLexer - " + error);

        return ret;
    }

    ScriptTokenList ScriptLexer::_tokenize(String str, const char* source, String& error)
    {
        // State enums
        enum{ READY = 0, COMMENT, MULTICOMMENT, WORD, QUOTE, VAR, POSSIBLECOMMENT };

        // Set up some constant characters of interest
        const wchar_t varopener = '$', quote = '\"', slash = '/', backslash = '\\', openbrace = '{', closebrace = '}', colon = ':', star = '*', cr = '\r', lf = '\n';
        char c = 0, lastc = 0;

        char* lexeme = 0;
        size_t lexemeLen = 0;
        uint32 line = 1, state = READY, lastQuote = 0, firstOpenBrace = 0, braceLayer = 0;
        ScriptTokenList tokens;

        // Iterate over the input
        char* i = &str[0];
        const char* end = str.data() + str.size();
        while(i != end)
        {
            lastc = c;
            c = *i;

            if(c == quote)
                lastQuote = line;
            
            if(state == READY || state == WORD || state == VAR)
            {
                if(c == openbrace)
                {
                    if(braceLayer == 0)
                        firstOpenBrace = line;
                    
                    braceLayer ++;
                }
                else if(c == closebrace)
                {
                    if (braceLayer == 0)
                    {
                        error = StringUtil::format(
                            "no matching open bracket '{' found for close bracket '}' at %s:%d", source,
                            line);
                        return tokens;
                    }

                    braceLayer --;
                }
            }
            
            
            switch(state)
            {
            case READY:
                if(c == slash && lastc == slash)
                {
                    // Comment start, clear out the lexeme
                    lexemeLen = 0;
                    state = COMMENT;
                }
                else if(c == star && lastc == slash)
                {
                    lexemeLen = 0;
                    state = MULTICOMMENT;
                }
                else if(c == quote)
                {
                    // Clear out the lexeme ready to be filled with quotes!
                    lexeme = i;
                    lexemeLen = 1;
                    state = QUOTE;
                }
                else if(c == varopener)
                {
                    // Set up to read in a variable
                    lexeme = i;
                    lexemeLen = 1;
                    state = VAR;
                }
                else if(isNewline(c))
                {
                    lexeme = i;
                    lexemeLen = 1;
                    setToken(lexeme, line, tokens, lexemeLen);
                }
                else if(!isWhitespace(c))
                {
                    lexeme = i;
                    lexemeLen = 1;
                    if(c == slash)
                        state = POSSIBLECOMMENT;
                    else
                        state = WORD;
                }
                break;
            case COMMENT:
                if(isNewline(c))
                {
                    lexeme = i;
                    lexemeLen = 1;
                    setToken(lexeme, line, tokens, lexemeLen);
                    state = READY;
                }
                break;
            case MULTICOMMENT:
                if(c == slash && lastc == star)
                    state = READY;
                break;
            case POSSIBLECOMMENT:
                if(c == slash && lastc == slash)
                {
                    lexemeLen = 0;
                    state = COMMENT;
                    break;  
                }
                else if(c == star && lastc == slash)
                {
                    lexemeLen = 0;
                    state = MULTICOMMENT;
                    break;
                }
                else
                {
                    state = WORD;
                    OGRE_FALLTHROUGH;
                }
            case WORD:
                if(isNewline(c))
                {
                    setToken(lexeme, line, tokens, lexemeLen);
                    lexeme = i;
                    lexemeLen = 1;
                    setToken(lexeme, line, tokens, lexemeLen);
                    state = READY;
                }
                else if(isWhitespace(c))
                {
                    setToken(lexeme, line, tokens, lexemeLen);
                    state = READY;
                }
                else if(c == openbrace || c == closebrace || c == colon)
                {
                    setToken(lexeme, line, tokens, lexemeLen);
                    lexeme = i;
                    lexemeLen = 1;
                    setToken(lexeme, line, tokens, lexemeLen);
                    state = READY;
                }
                else
                {
                    lexemeLen++;
                }
                break;
            case QUOTE:
                if(c != backslash)
                {
                    // Allow embedded quotes with escaping
                    if(c == quote && lastc == backslash)
                    {
                        lexemeLen++;
                    }
                    else if(c == quote)
                    {
                        lexemeLen++;
                        setToken(lexeme, line, tokens, lexemeLen);
                        state = READY;
                    }
                    else
                    {
                        // Backtrack here and allow a backslash normally within the quote
                        if(lastc == backslash)
                            lexemeLen++;
                        lexemeLen++;
                    }
                }
                break;
            case VAR:
                if(isNewline(c))
                {
                    setToken(lexeme, line, tokens, lexemeLen);
                    lexeme = i;
                    lexemeLen = 1;
                    setToken(lexeme, line, tokens, lexemeLen);
                    state = READY;
                }
                else if(isWhitespace(c))
                {
                    setToken(lexeme, line, tokens, lexemeLen);
                    state = READY;
                }
                else if(c == openbrace || c == closebrace || c == colon)
                {
                    setToken(lexeme, line, tokens, lexemeLen);
                    lexeme = i;
                    lexemeLen = 1;
                    setToken(lexeme, line, tokens, lexemeLen);
                    state = READY;
                }
                else
                {
                    lexemeLen++;
                }
                break;
            }

            // Separate check for newlines just to track line numbers
            if(c == cr || (c == lf && lastc != cr))
                line++;

            i++;
        }

        // Check for valid exit states
        if(state == WORD || state == VAR)
        {
            if(lexemeLen)
                setToken(lexeme, line, tokens, lexemeLen);
        }
        else
        {
            if(state == QUOTE)
            {
                error = StringUtil::format("no matching \" found for \" at %s:%d", source, lastQuote);
                return tokens;
            }
        }
        
        // Check that all opened brackets have been closed
        if (braceLayer == 1)
        {
            error = StringUtil::format("no matching closing bracket '}' for open bracket '{' at %s:%d",
                                       source, firstOpenBrace);
        }
        else if (braceLayer > 1)
        {
            error = StringUtil::format(
                "too many open brackets (%d) '{' without matching closing bracket '}' in %s", braceLayer,
                source);
        }
       
        return tokens;
    }

    void ScriptLexer::setToken(char* lexeme, Ogre::uint32 line, ScriptTokenList& tokens, size_t lexemeLen)
    {
        const char openBracket = '{', closeBracket = '}', colon = ':',
            quote = '\"', var = '$';

        ScriptToken token;
        token.line = line;
        bool ignore = false;

        // Check the user token map first
        if(lexemeLen == 1 && isNewline(lexeme[0]))
        {
            token.type = TID_NEWLINE;
            if(!tokens.empty() && tokens.back().type == TID_NEWLINE)
                ignore = true;
        }
        else if(lexemeLen == 1 && lexeme[0] == openBracket)
            token.type = TID_LBRACKET;
        else if(lexemeLen == 1 && lexeme[0] == closeBracket)
            token.type = TID_RBRACKET;
        else if(lexemeLen == 1 && lexeme[0] == colon)
            token.type = TID_COLON;
        else
        {
            // This is either a non-zero length phrase or quoted phrase
            if(lexemeLen >= 2 && lexeme[0] == quote && lexeme[lexemeLen] == quote)
            {
                token.type = TID_QUOTE;
            }
            else if(lexemeLen > 1 && lexeme[0] == var)
                token.type = TID_VARIABLE;
            else
            {
                token.type = TID_WORD;
            }

            token.lexeme = String(lexeme, lexemeLen);
            printf("%s\n", token.lexeme.c_str());
        }

        if(!ignore)
            tokens.push_back(token);
    }

    bool ScriptLexer::isWhitespace(Ogre::String::value_type c)
    {
        return c == ' ' || c == '\r' || c == '\t';
    }

    bool ScriptLexer::isNewline(Ogre::String::value_type c)
    {
        return c == '\n' || c == '\r';
    }

}

