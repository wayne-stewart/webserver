
BEGIN_TEST("url negative test: not initialized");
	ServerState state = {0};
	server_init_regex(&state);
	HttpContext http_context = {0};
	TEST_EXPECT_ONE(http_validate_url(&state, &http_context));
	TEST_EXPECT_STRINGS_EQUAL("request.uri is not initialized", LASTLOG);
END_TEST

BEGIN_TEST("url negative test: empty string")
	ServerState state = {0};
	server_init_regex(&state);
	HttpContext http_context = { .request = { .uri = "" } };
	TEST_EXPECT_ONE(http_validate_url(&state, &http_context));
	TEST_EXPECT_STRINGS_EQUAL("request.uri has length 0", LASTLOG);
END_TEST

BEGIN_TEST("url negative test: double dots (..)")
	ServerState state = {0};
	server_init_regex(&state);
	HttpContext http_context = { .request = { .uri = "/../asdf.html" } };
	TEST_EXPECT_ONE(http_validate_url(&state, &http_context));
	TEST_EXPECT_STRINGS_EQUAL("request.uri has double dots (..)", LASTLOG);
END_TEST

BEGIN_TEST("url negative test: hidden file attempt '/admin/.conf'")
	ServerState state = {0};
	server_init_regex(&state);
	HttpContext http_context = { .request = { .uri = "/admin/.conf" } };
	TEST_EXPECT_ONE(http_validate_url(&state, &http_context));
	TEST_EXPECT_STRINGS_EQUAL("request.uri client attempt to access dotfile", LASTLOG);
END_TEST

BEGIN_TEST("url positive test: '/'")
	ServerState state = {0};
	server_init_regex(&state);
	HttpContext http_context = { .request = { .uri = "/" } };
	TEST_EXPECT_ZERO(http_validate_url(&state, &http_context));
END_TEST


