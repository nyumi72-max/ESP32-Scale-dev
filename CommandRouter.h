#ifndef COMMANDROUTER_H
#define COMMANDROUTER_H

#include <Arduino.h>

// ハンドラは「自分が処理すべきコマンドならtrueを返す」契約。
// 該当しないコマンドはfalseを返して次のハンドラに委ねる。
typedef bool (*CommandHandler)(const String& cmd);

// 全ハンドラより先に評価される特別なゲート。
// falseを返すとその時点で処理済み扱いとなり、以降のハンドラは呼ばれない。
// (拒否メッセージの通知はガード自身の責務とする)
typedef bool (*CommandGuard)(const String& cmd);

// 文字列コマンドを登録順にハンドラへ振り分ける軽量ディスパッチャ。
class CommandRouter
{
public:

    static constexpr uint8_t MAX_HANDLERS = 12;

    CommandRouter();

    void setGuard(CommandGuard guard);

    bool addHandler(CommandHandler handler);

    // 該当ハンドラ(またはガード拒否)があれば実行してtrue、
    // どのハンドラも処理しなかった場合はfalse
    bool dispatch(const String& cmd);

private:

    CommandHandler _handlers[MAX_HANDLERS];

    int _handlerCount;

    CommandGuard _guard;
};

#endif
