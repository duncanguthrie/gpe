typedef struct
{
  gchar *nick;
  gchar *username;
  gchar *real_name;
  gchar *email;
  gchar *password;
} IRCUserInfo;

typedef struct
{
  gchar *name;
  GString *text;
  int fd;
  GIOChannel *io_channel;
  gboolean connected;
  GHashTable *channel;
  GtkWidget *button;
  IRCUserInfo *user_info;
} IRCServer;

typedef struct
{
  gchar *name;
  IRCServer *server;
  GString *text;
  gchar *topic;
  GtkWidget *button;
  GList *users;
} IRCChannel;

enum irc_type
{
  IRC_SERVER,
  IRC_CHANNEL
};

extern gint irc_server_read (IRCServer *server, gchar **passback_message);

extern gboolean irc_channel_send_message (IRCServer *server, gchar *message);

extern gboolean irc_server_join_channel (IRCServer *server, gchar *channel);

extern gboolean irc_server_login (IRCServer *server);

extern gboolean irc_server_connect (IRCServer *server);

extern gboolean irc_server_disconnect (IRCServer *server);

extern gboolean irc_quit (IRCServer *server, gchar *reason);

extern gboolean irc_part (IRCServer *server, gchar *channel, gchar *reason);
