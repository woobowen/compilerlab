%filenames = "scanner"

/*
 * Please don't modify the lines above.
 */

/* You can add lex definitions here. */
digit[0 - 9]
letter[a - zA - Z]

%x COMMENT STR IGNORE
/* a counter to support nested comments*/
/* int comment_depth = 0; */

%%

/*
 * Below is examples, which you can wipe out
 * and write regular expressions and actions of your own.
 *
 * All the tokens:
 *   Parser::ID
 *   Parser::STRING
 *   Parser::INT
 *   Parser::COMMA -
 *   Parser::COLON -
 *   Parser::SEMICOLON -
 *   Parser::LPAREN -
 *   Parser::RPAREN -
 *   Parser::LBRACK
 *   Parser::RBRACK
 *   Parser::LBRACE
 *   Parser::RBRACE
 *   Parser::DOT
 *   Parser::PLUS
 *   Parser::MINUS
 *   Parser::TIMES
 *   Parser::DIVIDE
 *   Parser::EQ
 *   Parser::NEQ
 *   Parser::LT
 *   Parser::LE
 *   Parser::GT
 *   Parser::GE
 *   Parser::AND
 *   Parser::OR
 *   Parser::ASSIGN
 *   Parser::ARRAY
 *   Parser::IF
 *   Parser::THEN
 *   Parser::ELSE
 *   Parser::WHILE
 *   Parser::FOR
 *   Parser::TO
 *   Parser::DO
 *   Parser::LET
 *   Parser::IN
 *   Parser::END
 *   Parser::OF
 *   Parser::BREAK
 *   Parser::NIL
 *   Parser::FUNCTION
 *   Parser::VAR
 *   Parser::TYPE
 */

/* reserved words */

"," {
  adjust();
  return Parser::COMMA;
}
":" {
  adjust();
  return Parser::COLON;
}
";" {
  adjust();
  return Parser::SEMICOLON;
}
"(" {
  adjust();
  return Parser::LPAREN;
}
")" {
  adjust();
  return Parser::RPAREN;
}
"[" {
  adjust();
  return Parser::LBRACK;
}
"]" {
  adjust();
  return Parser::RBRACK;
}
"{" {
  adjust();
  return Parser::LBRACE;
}
"}" {
  adjust();
  return Parser::RBRACE;
}
"." {
  adjust();
  return Parser::DOT;
}
"+" {
  adjust();
  return Parser::PLUS;
}
"-" {
  adjust();
  return Parser::MINUS;
}
"*" {
  adjust();
  return Parser::TIMES;
}
"/" {
  adjust();
  return Parser::DIVIDE;
}
"=" {
  adjust();
  return Parser::EQ;
}
/* why not != .., who tf designed this*/
"<>" {
  adjust();
  return Parser::NEQ;
}
"<" {
  adjust();
  return Parser::LT;
}
"<=" {
  adjust();
  return Parser::LE;
}
">" {
  adjust();
  return Parser::GT;
}
">=" {
  adjust();
  return Parser::GE;
}
"&" {
  adjust();
  return Parser::AND;
}
"|" {
  adjust();
  return Parser::OR;
}
":=" {
  adjust();
  return Parser::ASSIGN;
}

"array" {
  adjust();
  return Parser::ARRAY;
}

"if" {
  adjust();
  return Parser::IF;
}

"then" {
  adjust();
  return Parser::THEN;
}

"else" {
  adjust();
  return Parser::ELSE;
}

"while" {
  adjust();
  return Parser::WHILE;
}

"for" {
  adjust();
  return Parser::FOR;
}

"to" {
  adjust();
  return Parser::TO;
}

"do" {
  adjust();
  return Parser::DO;
}

"let" {
  adjust();
  return Parser::LET;
}

"in" {
  adjust();
  return Parser::IN;
}

"end" {
  adjust();
  return Parser::END;
}

"of" {
  adjust();
  return Parser::OF;
}

"break" {
  adjust();
  return Parser::BREAK;
}

"nil" {
  adjust();
  return Parser::NIL;
}

"function" {
  adjust();
  return Parser::FUNCTION;
}

"var" {
  adjust();
  return Parser::VAR;
}

"type" {
  adjust();
  return Parser::TYPE;
}

/* TODO: Put your lab2 code here */

/* skip comments */

"/*" {
  adjust();
  comment_level_ = 1;
  begin(StartCondition_::COMMENT);
}

<COMMENT> {
  "/*" {
      adjust();
      comment_level_ += 1;
  }
  "*/" {
      adjust();
      comment_level_ -= 1;
      if (comment_level_ == 0) {
          begin(StartCondition_::INITIAL);
      }
  }
  \n {
      adjust();
      errormsg_->Newline();
  }
  . {
      adjust();
  }
}

"\"" {
    adjust();
    begin(StartCondition_::STR);
    string_buf_ = "";
}

<STR> {
    \\[0-9]{3} {
        int code = std::stoi(std::string(matched().begin() + 1, matched().end()));
        /*
        if (code > 255) {
            errormsg_->Error(char_pos_, "^range");
        } else {
            string_buf_ += (char)code;
        }
        */
        string_buf_ += (char)code;
        adjustStr();
    }

    \\\" {
        adjustStr();
        string_buf_ += '\"';
    }

    \\\\ {
        adjustStr();
        string_buf_ += '\\';
    }

    \\n {
        adjustStr();
        string_buf_ += '\n';
    }

    \\t {
        adjustStr();
        string_buf_ += '\t';
    }

    \\[ \n\t\f\r]+\\ {
        adjustStr();
        for (char c : matched()) {
            if (c == '\n') errormsg_->Newline();
        }
    }

    "\\^"[@A-Z\[\\\]\^_] {
        adjustStr();
        string_buf_ += matched()[2] - '@';
    }

    \" {
        adjustStr();
        setMatched(string_buf_);
        begin(StartCondition_::INITIAL);
        return Parser::STRING;
    }

    . {
        adjustStr();
        string_buf_ += matched();
    }
}

[0-9]+ {
    adjust(); return Parser::INT;
}

[A-Za-z_][A-Za-z0-9_]* {
    adjust(); string_buf_=matched(); return Parser::ID;
}

/*
 * skip white space chars.
 * space, tabs and LF
 */
[ \t]+ {
  adjust();
}

\n {
  adjust();
  errormsg_->Newline();
}

/* illegal input */
. {
  adjust();
  errormsg_->Error(errormsg_->tok_pos_, "illegal token");
}
