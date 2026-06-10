#ifndef MarkdownParser_HPP_
#define MarkdownParser_HPP_

#include <QString>

class MarkdownParser {
public:
  static QString toHtml(const QString &markdown);

private:
  static QString escapeHtml(const QString &text);
  static QString parseInline(const QString &text);
  static bool isSafeLink(const QString &url);
  static int linkEndIndex(const QString &text, int start);
  static bool isWordChar(const QChar &character);
  static int closingDelimiterIndex(const QString &text,
                                   const QString &delimiter, int start);
};

#endif /* MarkdownParser_HPP_ */
