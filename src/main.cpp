#include "applicationui.hpp"

#include <bb/cascades/Application>
#include <bb/system/SystemToast>

#include <QDebug>
#include <QObject>
#include <QString>
#include <QTimerEvent>

extern "C" {
#include "mongoose.h"
}

using namespace bb::cascades;

static uint64_t s_next_heartbeat_ms = 0;
static long s_heartbeat_interval_ms = 0;
static struct mg_connection *s_ws_conn = NULL;

class MongoosePoller : public QObject {
public:
  explicit MongoosePoller(struct mg_mgr *mgr)
      : QObject(), m_mgr(mgr), m_timerId(0), m_closed(false) {
    m_timerId = startTimer(50);
  }

  ~MongoosePoller() { closeWebSocket(); }

  void closeWebSocket() {
    if (m_closed) {
      return;
    }

    m_closed = true;

    if (m_timerId != 0) {
      killTimer(m_timerId);
      m_timerId = 0;
    }

    if (s_ws_conn != NULL && !s_ws_conn->is_closing) {
      qDebug() << "[mg] closing websocket";

      if (s_ws_conn->is_websocket) {
        mg_ws_send(s_ws_conn, "", 0, WEBSOCKET_OP_CLOSE);
      }

      s_ws_conn->is_closing = 1;

      for (int i = 0; i < 10 && s_ws_conn != NULL; ++i) {
        mg_mgr_poll(m_mgr, 50);
      }
    }
  }

protected:
  void timerEvent(QTimerEvent *) {
    if (!m_closed) {
      mg_mgr_poll(m_mgr, 0);
    }
  }

private:
  struct mg_mgr *m_mgr;
  int m_timerId;
  bool m_closed;
};

static void toast(const QString &s) {
  qDebug() << "[toast]" << s;

  bb::system::SystemToast *t = new bb::system::SystemToast();
  t->setBody(s);
  t->show();
}

static const char *ev_name(int ev) {
  switch (ev) {
  case MG_EV_OPEN:
    return "MG_EV_OPEN";
  case MG_EV_POLL:
    return "MG_EV_POLL";
  case MG_EV_RESOLVE:
    return "MG_EV_RESOLVE";
  case MG_EV_CONNECT:
    return "MG_EV_CONNECT";
  case MG_EV_TLS_HS:
    return "MG_EV_TLS_HS";
  case MG_EV_ACCEPT:
    return "MG_EV_ACCEPT";
  case MG_EV_READ:
    return "MG_EV_READ";
  case MG_EV_WRITE:
    return "MG_EV_WRITE";
  case MG_EV_CLOSE:
    return "MG_EV_CLOSE";
  case MG_EV_ERROR:
    return "MG_EV_ERROR";
  case MG_EV_HTTP_MSG:
    return "MG_EV_HTTP_MSG";
  case MG_EV_WS_OPEN:
    return "MG_EV_WS_OPEN";
  case MG_EV_WS_MSG:
    return "MG_EV_WS_MSG";
  case MG_EV_WS_CTL:
    return "MG_EV_WS_CTL";
  default:
    return "UNKNOWN";
  }
}
static void wsfn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev != MG_EV_POLL) {
    qDebug() << "[mg]" << ev_name(ev) << "ev=" << ev << "fd=" << c->fd
             << "is_tls=" << c->is_tls << "is_websocket=" << c->is_websocket
             << "is_closing=" << c->is_closing
             << "recv.len=" << (unsigned int)c->recv.len
             << "send.len=" << (unsigned int)c->send.len;
  }

  switch (ev) {
  case MG_EV_OPEN:
    toast("Mongoose: open");
    break;

  case MG_EV_CONNECT:
    qDebug() << "[mg] tcp connected";
    toast("Mongoose: TCP connected");

    if (c->is_tls) {
      struct mg_tls_opts opts;
      memset(&opts, 0, sizeof(opts));
      opts.name = mg_str("gateway.discord.gg");
      opts.skip_verification = true;
      mg_tls_init(c, &opts);
    }
    break;

  case MG_EV_TLS_HS:
    qDebug() << "[mg] TLS handshake OK";
    toast("TLS OK");
    break;

  case MG_EV_ERROR:
    qDebug() << "[mg] error:" << (ev_data ? (char *)ev_data : "(null)");
    toast(QString("Mongoose error: %1")
              .arg(ev_data ? (char *)ev_data : "(null)"));
    break;

  case MG_EV_WS_OPEN:
    qDebug() << "[mg] websocket connected";
    toast("WS connected");
    break;

  case MG_EV_WS_MSG: {
    struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
    QString msg = QString::fromUtf8(wm->data.buf, (int)wm->data.len);
    int op = (int)mg_json_get_long(wm->data, "$.op", -1);

    qDebug() << "[mg] ws msg:" << msg;

    if (op == 10) {
      s_heartbeat_interval_ms =
          mg_json_get_long(wm->data, "$.d.heartbeat_interval", 0);
      s_next_heartbeat_ms = mg_millis() + s_heartbeat_interval_ms;

      qDebug() << "[discord] hello heartbeat_interval="
               << s_heartbeat_interval_ms;
      toast("Discord hello received");
    } else if (op == 11) {
      qDebug() << "[discord] heartbeat ACK";
    }
    break;
  }

  case MG_EV_POLL:
    if (c->is_websocket && s_heartbeat_interval_ms > 0 &&
        mg_millis() >= s_next_heartbeat_ms) {
      static const char heartbeat[] = "{\"op\":1,\"d\":null}";
      mg_ws_send(c, heartbeat, sizeof(heartbeat) - 1, WEBSOCKET_OP_TEXT);
      s_next_heartbeat_ms = mg_millis() + s_heartbeat_interval_ms;
      qDebug() << "[discord] heartbeat sent";
    }
    break;

  case MG_EV_CLOSE:
    qDebug() << "[mg] closed";
    s_ws_conn = NULL;
    s_heartbeat_interval_ms = 0;
    s_next_heartbeat_ms = 0;
    toast("Mongoose: closed");
    break;
  }
}

Q_DECL_EXPORT int main(int argc, char **argv) {
  Application app(argc, argv);

  qDebug() << "BBCord starting";
  qDebug() << "Mongoose version:" << MG_VERSION;

  mg_log_set(MG_LL_DEBUG);

  struct mg_mgr mgr;
  mg_mgr_init(&mgr);

  toast(QString("Mongoose %1").arg(MG_VERSION));

  const char *url = "wss://gateway.discord.gg/?v=10&encoding=json";

  qDebug() << "connecting to" << url;

  struct mg_connection *conn =
      mg_ws_connect(&mgr, "wss://gateway.discord.gg/?v=10&encoding=json", wsfn,
                    NULL, "User-Agent: BBCord/0.1\r\n");

  if (conn == NULL) {
    qDebug() << "mg_ws_connect returned NULL";
    toast("mg_ws_connect returned NULL");
  } else {
    qDebug() << "mg_ws_connect OK, conn =" << conn;
    s_ws_conn = conn;
  }

  MongoosePoller poller(&mgr);

  ApplicationUI appui;

  int rc = Application::exec();

  poller.closeWebSocket();
  mg_mgr_free(&mgr);

  return rc;
}