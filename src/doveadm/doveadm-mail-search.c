/* Copyright (c) 2010-2018 Dovecot authors, see the included COPYING file */

#include "lib.h"
#include "mail-storage.h"
#include "doveadm-print.h"
#include "doveadm-mailbox-list-iter.h"
#include "doveadm-mail-iter.h"
#include "doveadm-mail.h"

#include <stdio.h>

static int
cmd_search_box(struct doveadm_mail_cmd_context *ctx,
	       const struct mailbox_info *info)
{
	struct doveadm_mail_iter *iter;
	struct mailbox *box;
	struct mail *mail;
	struct mailbox_metadata metadata;
	const char *guid_str;

	int ret = doveadm_mail_iter_init(ctx, info, ctx->search_args, 0, NULL,
					 DOVEADM_MAIL_ITER_FLAG_STOP_WITH_CLIENT,
					 &iter);
	if (ret <= 0)
		return ret;

	box = doveadm_mail_iter_get_mailbox(iter);

	ret = 0;
	if (mailbox_get_metadata(box, MAILBOX_METADATA_GUID, &metadata) < 0) {
		i_error("Couldn't get mailbox '%s' GUID: %s",
			mailbox_get_vname(box),
			mailbox_get_last_internal_error(box, NULL));
		ret = -1;
		doveadm_mail_failed_mailbox(ctx, box);
	} else {
		guid_str = guid_128_to_string(metadata.guid);
		while (doveadm_mail_iter_next(iter, &mail)) {
			doveadm_print(guid_str);
			T_BEGIN {
				doveadm_print(dec2str(mail->uid));
			} T_END;
		}
	}
	if (doveadm_mail_iter_deinit(&iter) < 0)
		ret = -1;
	return ret;
}

static int
cmd_search_run(struct doveadm_mail_cmd_context *ctx, struct mail_user *user)
{
	const enum mailbox_list_iter_flags iter_flags =
		MAILBOX_LIST_ITER_NO_AUTO_BOXES |
		MAILBOX_LIST_ITER_RETURN_NO_FLAGS;
	struct doveadm_mailbox_list_iter *iter;
	const struct mailbox_info *info;
	int ret = 0;

	iter = doveadm_mailbox_list_iter_init(ctx, user, ctx->search_args,
					      iter_flags);
	while ((info = doveadm_mailbox_list_iter_next(iter)) != NULL) T_BEGIN {
		if (cmd_search_box(ctx, info) < 0)
			ret = -1;
	} T_END;
	if (doveadm_mailbox_list_iter_deinit(&iter) < 0)
		ret = -1;
	return ret;
}

static void cmd_search_init(struct doveadm_mail_cmd_context *ctx,
			    const char *const args[])
{
	if (args[0] == NULL)
		doveadm_mail_help_name("search");

	doveadm_print_header("mailbox-guid", "mailbox-guid",
			     DOVEADM_PRINT_HEADER_FLAG_HIDE_TITLE);
	doveadm_print_header("uid", "uid",
			     DOVEADM_PRINT_HEADER_FLAG_HIDE_TITLE);

	ctx->search_args = doveadm_mail_build_search_args(args);
}

static struct doveadm_mail_cmd_context *cmd_search_alloc(void)
{
	struct doveadm_mail_cmd_context *ctx;

	ctx = doveadm_mail_cmd_alloc(struct doveadm_mail_cmd_context);
	ctx->v.init = cmd_search_init;
	ctx->v.run = cmd_search_run;
	doveadm_print_init(DOVEADM_PRINT_TYPE_FLOW);
	return ctx;
}

struct doveadm_cmd_ver2 doveadm_cmd_search_ver2 = {
	.name = "search",
	.mail_cmd = cmd_search_alloc,
	.usage = DOVEADM_CMD_MAIL_USAGE_PREFIX "<search query>",
DOVEADM_CMD_PARAMS_START
DOVEADM_CMD_MAIL_COMMON
DOVEADM_CMD_PARAM('\0', "query", CMD_PARAM_ARRAY, CMD_PARAM_FLAG_POSITIONAL)
DOVEADM_CMD_PARAMS_END
};
