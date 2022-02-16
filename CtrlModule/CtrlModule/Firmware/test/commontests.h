int tests_run = 0;
int tests_passed = 0;


#define teststart(a, s) printf("----------\nTest %s\n\n", s); r = a
#define testend(a, s, r) tests_passed += (r) ? 1 : 0; tests_run ++;  printf("Test %s result %s\n=======\n", s, (r) ? "PASS" : "!FAIL!")
#define passif(a, s) teststart(a,s); fprintf(stderr, "[%d] %s - %s\n", __LINE__, (r) ? "PASS" : "!FAIL!", s); testend(a,s,r)
#define passifeq(a, b, s) teststart(a,s); if ((r) != (b)) fprintf(stderr, "[%d] !FAIL! - %s - expected %d got %d\n", __LINE__, s, (b), (r)); else fprintf(stderr, "[%d] PASS - %s\n", __LINE__, s); testend(a,s,(r) == (b))
#define passifstreq(a, b, s) teststart(!strcmp(a,b),s); if (!r) fprintf(stderr, "[%d] !FAIL! - %s - expected %s got %s\n", __LINE__, s, (b), (a)); else fprintf(stderr, "[%d] PASS - %s\n", __LINE__, s); testend(a,s,r)
#define passifeqh(a, b, s) teststart(a,s); if ((r) != (b)) fprintf(stderr, "[%d] !FAIL! - %s - expected %x got %x\n", __LINE__, s, (b), (r)); else fprintf(stderr, "[%d] PASS - %s\n", __LINE__, s); testend(a,s,(r) == (b))

void infotests() {
  printf("tests_run = %d\n", tests_run);
  printf("tests_passed = %d\n", tests_passed);

  fprintf(stderr, "tests_run = %d\n", tests_run);
  fprintf(stderr, "tests_passed = %d\n", tests_passed);
}
