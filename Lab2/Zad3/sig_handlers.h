#ifndef SIG_HANDLERS_H
#define SIG_HANDLERS_H

void obsluga(int signum);
void sig_default();
void sig_mask();
void sig_ignore();
void sig_handle();
void sig_unblock();

#endif