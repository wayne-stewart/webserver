
BEGIN_TEST("url not initialized");
	ServerState state = {0};
	server_init_regex(&state);
	HttpContext http_context = {0};
	TEST_EXPECT_ONE(http_validate_url(&state, &http_context));
	TEST_EXPECT_STRINGS_EQUAL("request.uri is not initialized", LASTLOG);
END_TEST

BEGIN_TEST("url empty string")
	ServerState state = {0};
	server_init_regex(&state);
	HttpContext http_context = { .request = { .uri = "" } };
	TEST_EXPECT_ONE(http_validate_url(&state, &http_context));
	TEST_EXPECT_STRINGS_EQUAL("request.uri has length 0", LASTLOG);
END_TEST

BEGIN_TEST("url test minimal '/'")
	ServerState state = {0};
	server_init_regex(&state);
	HttpContext http_context = { .request = { .uri = "/" } };
	TEST_EXPECT_ZERO(http_validate_url(&state, &http_context));
END_TEST
