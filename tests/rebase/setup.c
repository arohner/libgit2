#include "clar_libgit2.h"
#include "git2/rebase.h"
#include "posix.h"

#include <fcntl.h>

static git_repository *repo;
static git_index *_index;
static git_signature *signature;

// Fixture setup and teardown
void test_rebase_setup__initialize(void)
{
	repo = cl_git_sandbox_init("rebase");
	cl_git_pass(git_repository_index(&_index, repo));
	cl_git_pass(git_signature_now(&signature, "Rebaser", "rebaser@rebaser.rb"));
}

void test_rebase_setup__cleanup(void)
{
	git_signature_free(signature);
	git_index_free(_index);
	cl_git_sandbox_cleanup();
}

/* git checkout beef ; git rebase --merge master
 * git checkout beef ; git rebase --merge master */
void test_rebase_setup__blocked_when_in_progress(void)
{
	git_reference *branch_ref, *upstream_ref;
	git_merge_head *branch_head, *upstream_head;

	cl_assert_equal_i(GIT_REPOSITORY_STATE_NONE, git_repository_state(repo));

	cl_git_pass(git_reference_lookup(&branch_ref, repo, "refs/heads/beef"));
	cl_git_pass(git_reference_lookup(&upstream_ref, repo, "refs/heads/master"));

	cl_git_pass(git_merge_head_from_ref(&branch_head, repo, branch_ref));
	cl_git_pass(git_merge_head_from_ref(&upstream_head, repo, upstream_ref));

	cl_git_pass(git_rebase(repo, branch_head, upstream_head, NULL, signature, NULL));

	cl_assert_equal_i(GIT_REPOSITORY_STATE_REBASE_MERGE, git_repository_state(repo));

	cl_git_fail(git_rebase(repo, branch_head, upstream_head, NULL, signature, NULL));

	git_merge_head_free(branch_head);
	git_merge_head_free(upstream_head);
	git_reference_free(branch_ref);
	git_reference_free(upstream_ref);
}

/* git checkout beef ; git rebase --merge master */
void test_rebase_setup__merge(void)
{
	git_reference *branch_ref, *upstream_ref;
	git_merge_head *branch_head, *upstream_head;
	git_reference *head;
	git_commit *head_commit;
	git_oid head_id;

	cl_assert_equal_i(GIT_REPOSITORY_STATE_NONE, git_repository_state(repo));

	cl_git_pass(git_reference_lookup(&branch_ref, repo, "refs/heads/beef"));
	cl_git_pass(git_reference_lookup(&upstream_ref, repo, "refs/heads/master"));

	cl_git_pass(git_merge_head_from_ref(&branch_head, repo, branch_ref));
	cl_git_pass(git_merge_head_from_ref(&upstream_head, repo, upstream_ref));

	cl_git_pass(git_rebase(repo, branch_head, upstream_head, NULL, signature, NULL));

	cl_assert_equal_i(GIT_REPOSITORY_STATE_REBASE_MERGE, git_repository_state(repo));

	git_oid_fromstr(&head_id, "efad0b11c47cb2f0220cbd6f5b0f93bb99064b00");
	cl_git_pass(git_repository_head(&head, repo));
	cl_git_pass(git_reference_peel((git_object **)&head_commit, head, GIT_OBJ_COMMIT));
	cl_assert_equal_oid(&head_id, git_commit_id(head_commit));

	cl_assert_equal_file("b146bd7608eac53d9bf9e1a6963543588b555c64\n", 41, "rebase/.git/ORIG_HEAD");

	cl_assert_equal_file("da9c51a23d02d931a486f45ad18cda05cf5d2b94\n", 41, "rebase/.git/rebase-merge/cmt.1");
	cl_assert_equal_file("8d1f13f93c4995760ac07d129246ac1ff64c0be9\n", 41, "rebase/.git/rebase-merge/cmt.2");
	cl_assert_equal_file("3069cc907e6294623e5917ef6de663928c1febfb\n", 41, "rebase/.git/rebase-merge/cmt.3");
	cl_assert_equal_file("588e5d2f04d49707fe4aab865e1deacaf7ef6787\n", 41, "rebase/.git/rebase-merge/cmt.4");
	cl_assert_equal_file("b146bd7608eac53d9bf9e1a6963543588b555c64\n", 41, "rebase/.git/rebase-merge/cmt.5");
	cl_assert_equal_file("5\n", 2, "rebase/.git/rebase-merge/end");
	cl_assert_equal_file("efad0b11c47cb2f0220cbd6f5b0f93bb99064b00\n", 41, "rebase/.git/rebase-merge/onto");
	cl_assert_equal_file("master\n", 7, "rebase/.git/rebase-merge/onto_name");
	cl_assert_equal_file("b146bd7608eac53d9bf9e1a6963543588b555c64\n", 41, "rebase/.git/rebase-merge/orig-head");

	git_commit_free(head_commit);
	git_reference_free(head);
	git_merge_head_free(branch_head);
	git_merge_head_free(upstream_head);
	git_reference_free(branch_ref);
	git_reference_free(upstream_ref);
}

/* git checkout beef && git rebase --merge --root --onto master */
void test_rebase_setup__merge_root(void)
{
	git_reference *branch_ref, *onto_ref;
	git_merge_head *branch_head, *onto_head;
	git_reference *head;
	git_commit *head_commit;
	git_oid head_id;

	cl_assert_equal_i(GIT_REPOSITORY_STATE_NONE, git_repository_state(repo));

	cl_git_pass(git_reference_lookup(&branch_ref, repo, "refs/heads/beef"));
	cl_git_pass(git_reference_lookup(&onto_ref, repo, "refs/heads/master"));

	cl_git_pass(git_merge_head_from_ref(&branch_head, repo, branch_ref));
	cl_git_pass(git_merge_head_from_ref(&onto_head, repo, onto_ref));

	cl_git_pass(git_rebase(repo, branch_head, NULL, onto_head, signature, NULL));

	git_oid_fromstr(&head_id, "efad0b11c47cb2f0220cbd6f5b0f93bb99064b00");
	cl_git_pass(git_repository_head(&head, repo));
	cl_git_pass(git_reference_peel((git_object **)&head_commit, head, GIT_OBJ_COMMIT));
	cl_assert_equal_oid(&head_id, git_commit_id(head_commit));

	cl_assert_equal_file("b146bd7608eac53d9bf9e1a6963543588b555c64\n", 41, "rebase/.git/ORIG_HEAD");

	cl_assert_equal_i(GIT_REPOSITORY_STATE_REBASE_MERGE, git_repository_state(repo));

	cl_assert_equal_file("da9c51a23d02d931a486f45ad18cda05cf5d2b94\n", 41, "rebase/.git/rebase-merge/cmt.1");
	cl_assert_equal_file("8d1f13f93c4995760ac07d129246ac1ff64c0be9\n", 41, "rebase/.git/rebase-merge/cmt.2");
	cl_assert_equal_file("3069cc907e6294623e5917ef6de663928c1febfb\n", 41, "rebase/.git/rebase-merge/cmt.3");
	cl_assert_equal_file("588e5d2f04d49707fe4aab865e1deacaf7ef6787\n", 41, "rebase/.git/rebase-merge/cmt.4");
	cl_assert_equal_file("b146bd7608eac53d9bf9e1a6963543588b555c64\n", 41, "rebase/.git/rebase-merge/cmt.5");
	cl_assert_equal_file("5\n", 2, "rebase/.git/rebase-merge/end");
	cl_assert_equal_file("efad0b11c47cb2f0220cbd6f5b0f93bb99064b00\n", 41, "rebase/.git/rebase-merge/onto");
	cl_assert_equal_file("master\n", 7, "rebase/.git/rebase-merge/onto_name");
	cl_assert_equal_file("b146bd7608eac53d9bf9e1a6963543588b555c64\n", 41, "rebase/.git/rebase-merge/orig-head");

	git_commit_free(head_commit);
	git_reference_free(head);
	git_merge_head_free(branch_head);
	git_merge_head_free(onto_head);
	git_reference_free(branch_ref);
	git_reference_free(onto_ref);
}

/* git checkout gravy && git rebase --merge --onto master veal */
void test_rebase_setup__merge_onto_and_upstream(void)
{
	git_reference *branch1_ref, *branch2_ref, *onto_ref;
	git_merge_head *branch1_head, *branch2_head, *onto_head;
	git_reference *head;
	git_commit *head_commit;
	git_oid head_id;

	cl_assert_equal_i(GIT_REPOSITORY_STATE_NONE, git_repository_state(repo));

	cl_git_pass(git_reference_lookup(&branch1_ref, repo, "refs/heads/gravy"));
	cl_git_pass(git_reference_lookup(&branch2_ref, repo, "refs/heads/veal"));
	cl_git_pass(git_reference_lookup(&onto_ref, repo, "refs/heads/master"));

	cl_git_pass(git_merge_head_from_ref(&branch1_head, repo, branch1_ref));
	cl_git_pass(git_merge_head_from_ref(&branch2_head, repo, branch2_ref));
	cl_git_pass(git_merge_head_from_ref(&onto_head, repo, onto_ref));

	cl_git_pass(git_rebase(repo, branch1_head, branch2_head, onto_head, signature, NULL));

	git_oid_fromstr(&head_id, "efad0b11c47cb2f0220cbd6f5b0f93bb99064b00");
	cl_git_pass(git_repository_head(&head, repo));
	cl_git_pass(git_reference_peel((git_object **)&head_commit, head, GIT_OBJ_COMMIT));
	cl_assert_equal_oid(&head_id, git_commit_id(head_commit));

	cl_assert_equal_file("d616d97082eb7bb2dc6f180a7cca940993b7a56f\n", 41, "rebase/.git/ORIG_HEAD");

	cl_assert_equal_i(GIT_REPOSITORY_STATE_REBASE_MERGE, git_repository_state(repo));

	cl_assert_equal_file("d616d97082eb7bb2dc6f180a7cca940993b7a56f\n", 41, "rebase/.git/rebase-merge/cmt.1");
	cl_assert_equal_file("1\n", 2, "rebase/.git/rebase-merge/end");
	cl_assert_equal_file("efad0b11c47cb2f0220cbd6f5b0f93bb99064b00\n", 41, "rebase/.git/rebase-merge/onto");
	cl_assert_equal_file("master\n", 7, "rebase/.git/rebase-merge/onto_name");
	cl_assert_equal_file("d616d97082eb7bb2dc6f180a7cca940993b7a56f\n", 41, "rebase/.git/rebase-merge/orig-head");

	git_commit_free(head_commit);
	git_reference_free(head);
	git_merge_head_free(branch1_head);
	git_merge_head_free(branch2_head);
	git_merge_head_free(onto_head);
	git_reference_free(branch1_ref);
	git_reference_free(branch2_ref);
	git_reference_free(onto_ref);
}

/* Ensure merge commits are dropped in a rebase */
/* git checkout veal && git rebase --merge master */
void test_rebase_setup__branch_with_merges(void)
{
	git_reference *branch_ref, *upstream_ref;
	git_merge_head *branch_head, *upstream_head;
	git_reference *head;
	git_commit *head_commit;
	git_oid head_id;

	cl_assert_equal_i(GIT_REPOSITORY_STATE_NONE, git_repository_state(repo));

	cl_git_pass(git_reference_lookup(&branch_ref, repo, "refs/heads/veal"));
	cl_git_pass(git_reference_lookup(&upstream_ref, repo, "refs/heads/master"));

	cl_git_pass(git_merge_head_from_ref(&branch_head, repo, branch_ref));
	cl_git_pass(git_merge_head_from_ref(&upstream_head, repo, upstream_ref));

	cl_git_pass(git_rebase(repo, branch_head, upstream_head, NULL, signature, NULL));

	cl_assert_equal_i(GIT_REPOSITORY_STATE_REBASE_MERGE, git_repository_state(repo));

	git_oid_fromstr(&head_id, "efad0b11c47cb2f0220cbd6f5b0f93bb99064b00");
	cl_git_pass(git_repository_head(&head, repo));
	cl_git_pass(git_reference_peel((git_object **)&head_commit, head, GIT_OBJ_COMMIT));
	cl_assert_equal_oid(&head_id, git_commit_id(head_commit));

	cl_assert_equal_file("f87d14a4a236582a0278a916340a793714256864\n", 41, "rebase/.git/ORIG_HEAD");

	cl_assert_equal_file("4bed71df7017283cac61bbf726197ad6a5a18b84\n", 41, "rebase/.git/rebase-merge/cmt.1");
	cl_assert_equal_file("2aa3ce842094e08ebac152b3d6d5b0fff39f9c6e\n", 41, "rebase/.git/rebase-merge/cmt.2");
	cl_assert_equal_file("3e8989b5a16d5258c935d998ef0e6bb139cc4757\n", 41, "rebase/.git/rebase-merge/cmt.3");
	cl_assert_equal_file("4cacc6f6e740a5bc64faa33e04b8ef0733d8a127\n", 41, "rebase/.git/rebase-merge/cmt.4");
	cl_assert_equal_file("f87d14a4a236582a0278a916340a793714256864\n", 41, "rebase/.git/rebase-merge/cmt.5");
	cl_assert_equal_file("5\n", 2, "rebase/.git/rebase-merge/end");
	cl_assert_equal_file("efad0b11c47cb2f0220cbd6f5b0f93bb99064b00\n", 41, "rebase/.git/rebase-merge/onto");
	cl_assert_equal_file("master\n", 7, "rebase/.git/rebase-merge/onto_name");
	cl_assert_equal_file("f87d14a4a236582a0278a916340a793714256864\n", 41, "rebase/.git/rebase-merge/orig-head");

	git_commit_free(head_commit);
	git_reference_free(head);
	git_merge_head_free(branch_head);
	git_merge_head_free(upstream_head);
	git_reference_free(branch_ref);
	git_reference_free(upstream_ref);
}

/* git checkout barley && git rebase --merge master */
void test_rebase_setup__orphan_branch(void)
{
	git_reference *branch_ref, *upstream_ref;
	git_merge_head *branch_head, *upstream_head;
	git_reference *head;
	git_commit *head_commit;
	git_oid head_id;

	cl_assert_equal_i(GIT_REPOSITORY_STATE_NONE, git_repository_state(repo));

	cl_git_pass(git_reference_lookup(&branch_ref, repo, "refs/heads/barley"));
	cl_git_pass(git_reference_lookup(&upstream_ref, repo, "refs/heads/master"));

	cl_git_pass(git_merge_head_from_ref(&branch_head, repo, branch_ref));
	cl_git_pass(git_merge_head_from_ref(&upstream_head, repo, upstream_ref));

	cl_git_pass(git_rebase(repo, branch_head, upstream_head, NULL, signature, NULL));

	cl_assert_equal_i(GIT_REPOSITORY_STATE_REBASE_MERGE, git_repository_state(repo));

	git_oid_fromstr(&head_id, "efad0b11c47cb2f0220cbd6f5b0f93bb99064b00");
	cl_git_pass(git_repository_head(&head, repo));
	cl_git_pass(git_reference_peel((git_object **)&head_commit, head, GIT_OBJ_COMMIT));
	cl_assert_equal_oid(&head_id, git_commit_id(head_commit));

	cl_assert_equal_file("12c084412b952396962eb420716df01022b847cc\n", 41, "rebase/.git/ORIG_HEAD");

	cl_assert_equal_file("aa4c42aecdfc7cd989bbc3209934ea7cda3f4d88\n", 41, "rebase/.git/rebase-merge/cmt.1");
	cl_assert_equal_file("e4f809f826c1a9fc929874bc0e4644dd2f2a1af4\n", 41, "rebase/.git/rebase-merge/cmt.2");
	cl_assert_equal_file("9539b2cc291d6a6b1b266df8474d31fdd344dd79\n", 41, "rebase/.git/rebase-merge/cmt.3");
	cl_assert_equal_file("013cc32d341bab0e6f039f50f153c18986f16c58\n", 41, "rebase/.git/rebase-merge/cmt.4");
	cl_assert_equal_file("12c084412b952396962eb420716df01022b847cc\n", 41, "rebase/.git/rebase-merge/cmt.5");
	cl_assert_equal_file("5\n", 2, "rebase/.git/rebase-merge/end");
	cl_assert_equal_file("efad0b11c47cb2f0220cbd6f5b0f93bb99064b00\n", 41, "rebase/.git/rebase-merge/onto");
	cl_assert_equal_file("master\n", 7, "rebase/.git/rebase-merge/onto_name");
	cl_assert_equal_file("12c084412b952396962eb420716df01022b847cc\n", 41, "rebase/.git/rebase-merge/orig-head");

	git_commit_free(head_commit);
	git_reference_free(head);
	git_merge_head_free(branch_head);
	git_merge_head_free(upstream_head);
	git_reference_free(branch_ref);
	git_reference_free(upstream_ref);
}

static int rebase_is_blocked(void)
{
	int error;

	git_reference *branch_ref, *upstream_ref;
	git_merge_head *branch_head, *upstream_head;
				 
	cl_assert_equal_i(GIT_REPOSITORY_STATE_NONE, git_repository_state(repo));
						  
	cl_git_pass(git_reference_lookup(&branch_ref, repo, "refs/heads/beef"));
	cl_git_pass(git_reference_lookup(&upstream_ref, repo, "refs/heads/master"));
								   
	cl_git_pass(git_merge_head_from_ref(&branch_head, repo, branch_ref));
	cl_git_pass(git_merge_head_from_ref(&upstream_head, repo, upstream_ref));
												    
	error = git_rebase(repo, branch_head, upstream_head, NULL, signature, NULL);

	git_merge_head_free(branch_head);
	git_merge_head_free(upstream_head);

	git_reference_free(branch_ref);
	git_reference_free(upstream_ref);

	return error;
}

void test_rebase_setup__blocked_for_staged_change(void)
{
	cl_git_rewritefile("rebase/newfile.txt", "Stage an add");
	git_index_add_bypath(_index, "newfile.txt");
	cl_git_fail(rebase_is_blocked());
}

void test_rebase_setup__blocked_for_unstaged_change(void)
{
	cl_git_rewritefile("rebase/asparagus.txt", "Unstaged change");
	cl_git_fail(rebase_is_blocked());
}

void test_rebase_setup__not_blocked_for_untracked_add(void)
{
	cl_git_rewritefile("rebase/newfile.txt", "Untracked file");
	cl_git_pass(rebase_is_blocked());
}

