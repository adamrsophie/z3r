// A light scan of SMT-LIB text before and after Z3 parses it, for what
// Z3's parser does not report: the datatypes the text declares, the labels
// of :named assertions and options that change an optimizer.
#include "z3r.h"

#include <fstream>
#include <sstream>

namespace {

enum class Tok { Open, Close, Symbol, Keyword, Other };

struct Token {
  Tok kind;
  std::string text;  // a symbol without its bars
};

std::vector<Token> tokenize(const std::string& s) {
  std::vector<Token> out;
  size_t i = 0, n = s.size();
  auto is_space = [](char ch) { return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f'; };
  while (i < n) {
    char ch = s[i];
    if (is_space(ch)) {
      ++i;
    } else if (ch == ';') {
      while (i < n && s[i] != '\n') ++i;
    } else if (ch == '(') {
      out.push_back({Tok::Open, "("});
      ++i;
    } else if (ch == ')') {
      out.push_back({Tok::Close, ")"});
      ++i;
    } else if (ch == '"') {
      // A string literal; "" is an escaped quote.
      ++i;
      while (i < n) {
        if (s[i] == '"') {
          if (i + 1 < n && s[i + 1] == '"') {
            i += 2;
            continue;
          }
          ++i;
          break;
        }
        ++i;
      }
      out.push_back({Tok::Other, ""});
    } else if (ch == '|') {
      size_t j = s.find('|', i + 1);
      if (j == std::string::npos) j = n;
      out.push_back({Tok::Symbol, s.substr(i + 1, j - i - 1)});
      i = j + 1;
    } else {
      size_t j = i;
      while (j < n && !is_space(s[j]) && s[j] != '(' && s[j] != ')' && s[j] != ';' && s[j] != '"' && s[j] != '|') ++j;
      std::string word = s.substr(i, j - i);
      Tok kind = word[0] == ':' ? Tok::Keyword : Tok::Symbol;
      out.push_back({kind, word});
      i = j;
    }
  }
  return out;
}

// Index just past the expression that starts at token i.
size_t skip(const std::vector<Token>& t, size_t i) {
  if (i >= t.size()) return i;
  if (t[i].kind != Tok::Open) return i + 1;
  int depth = 0;
  for (; i < t.size(); ++i) {
    if (t[i].kind == Tok::Open) ++depth;
    if (t[i].kind == Tok::Close && --depth == 0) return i + 1;
  }
  return i;
}

bool is(const std::vector<Token>& t, size_t i, Tok kind) { return i < t.size() && t[i].kind == kind; }

}  // namespace

namespace z3r {

// Z3 replaces an earlier datatype of the same name and frees its sorts and
// functions, while R objects may still use them; refuse a second declaration
// made through z3r in the same context.
void refuse_redeclared(const CtxPtr& ctx, const std::string& name) {
  if (ctx->datatype_names.count(name)) {
    fail("a datatype named '" + name + "' is already declared in this context, and declaring it again "
         "would free the sorts and functions still in use; keep the earlier object, or use another "
         "name or a new context");
  }
}

std::string smt_file_text(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream text;
  if (in) text << in.rdbuf();
  return text.str();
}

std::vector<std::string> smt_datatype_names(const std::string& text) {
  std::vector<Token> t = tokenize(text);
  std::vector<std::string> out;
  for (size_t i = 0; i + 1 < t.size(); ++i) {
    if (t[i].kind != Tok::Open || t[i + 1].kind != Tok::Symbol) continue;
    const std::string& cmd = t[i + 1].text;
    size_t j = i + 2;
    if (cmd == "declare-datatype" || cmd == "declare-codatatype") {
      if (is(t, j, Tok::Symbol)) out.push_back(t[j].text);
    } else if (cmd == "declare-datatypes" || cmd == "declare-codatatypes") {
      if (!is(t, j, Tok::Open)) continue;
      if (is(t, j + 1, Tok::Close)) {
        // Z3's older form: (declare-datatypes () ((Name ctor ...) ...)).
        j += 2;
        if (!is(t, j, Tok::Open)) continue;
        for (size_t k = j + 1; is(t, k, Tok::Open);) {
          if (is(t, k + 1, Tok::Symbol)) out.push_back(t[k + 1].text);
          k = skip(t, k);
        }
      } else {
        // SMT-LIB 2.6: (declare-datatypes ((Name arity) ...) (...)).
        for (size_t k = j + 1; is(t, k, Tok::Open);) {
          if (is(t, k + 1, Tok::Symbol)) out.push_back(t[k + 1].text);
          k = skip(t, k);
        }
      }
    }
  }
  return out;
}

std::set<std::string> smt_named_labels(const std::string& text) {
  std::vector<Token> t = tokenize(text);
  std::set<std::string> out;
  for (size_t i = 0; i + 1 < t.size(); ++i) {
    if (t[i].kind == Tok::Keyword && t[i].text == ":named" && t[i + 1].kind == Tok::Symbol) out.insert(t[i + 1].text);
  }
  return out;
}

std::vector<std::string> smt_soft_weights(const std::string& text) {
  std::vector<Token> t = tokenize(text);
  std::vector<std::string> out;
  for (size_t i = 0; i + 1 < t.size(); ++i) {
    if (t[i].kind == Tok::Keyword && t[i].text == ":weight" && t[i + 1].kind == Tok::Symbol) out.push_back(t[i + 1].text);
  }
  return out;
}

bool smt_sets_option(const std::string& text, const std::string& option) {
  std::vector<Token> t = tokenize(text);
  for (size_t i = 0; i + 2 < t.size(); ++i) {
    if (t[i].kind == Tok::Open && t[i + 1].kind == Tok::Symbol && t[i + 1].text == "set-option" &&
        t[i + 2].kind == Tok::Keyword && t[i + 2].text == option) {
      return true;
    }
  }
  return false;
}

// Z3 keys datatypes by name in a context, and a second declaration replaces
// the first, changing the meaning of terms already built; z3r refuses text
// that declares a datatype name the context already has.
void guard_smt_datatypes(const CtxPtr& ctx, const std::string& text) {
  for (const std::string& name : smt_datatype_names(text)) refuse_redeclared(ctx, name);
}

void register_smt_datatypes(const CtxPtr& ctx, const std::string& text) {
  for (const std::string& name : smt_datatype_names(text)) ctx->datatype_names.insert(name);
}

}  // namespace z3r
