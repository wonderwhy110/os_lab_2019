#include <CUnit/Basic.h>
#include <stdio.h>
#include <string.h>

#include "revert_string.h"

void testRevertString(void) {
  char simple_string[] = "Hello";
  char str_with_spaces[] = "String with spaces";
  char str_with_odd_chars_num[] = "abc";
  char str_with_even_chars_num[] = "abcd";

  RevertString(simple_string);
  CU_ASSERT_STRING_EQUAL_FATAL(simple_string, "olleH");

  RevertString(str_with_spaces);
  CU_ASSERT_STRING_EQUAL_FATAL(str_with_spaces, "secaps htiw gnirtS");

  RevertString(str_with_odd_chars_num);
  CU_ASSERT_STRING_EQUAL_FATAL(str_with_odd_chars_num, "cba");

  RevertString(str_with_even_chars_num);
  CU_ASSERT_STRING_EQUAL_FATAL(str_with_even_chars_num, "dcba");
}

// Новая тестовая функция для другого набора тестов
void testEdgeCases(void) {
  char empty_string[] = "";
  char single_char[] = "a";
  char two_chars[] = "ab";
  
  // Тест пустой строки
  RevertString(empty_string);
  CU_ASSERT_STRING_EQUAL_FATAL(empty_string, "");
  
  // Тест одного символа
  RevertString(single_char);
  CU_ASSERT_STRING_EQUAL_FATAL(single_char, "a");
  
  // Тест двух символов
  RevertString(two_chars);
  CU_ASSERT_STRING_EQUAL_FATAL(two_chars, "ba");
}

// Еще одна тестовая функция
void testSpecialCharacters(void) {
  char with_numbers[] = "12345";
  char with_special[] = "a!b@c#";
  
  RevertString(with_numbers);
  CU_ASSERT_STRING_EQUAL_FATAL(with_numbers, "54321");
  
  RevertString(with_special);
  CU_ASSERT_STRING_EQUAL_FATAL(with_special, "#c@b!a");
}

int main() {
  CU_pSuite pSuite1 = NULL;
  CU_pSuite pSuite2 = NULL;
  CU_pSuite pSuite3 = NULL;

  /* initialize the CUnit test registry */
  if (CUE_SUCCESS != CU_initialize_registry()) 
    return CU_get_error();

  /* Первый suite - основные тесты */
  pSuite1 = CU_add_suite("Basic String Tests", NULL, NULL);
  if (NULL == pSuite1) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  /* Второй suite - граничные случаи */
  pSuite2 = CU_add_suite("Edge Cases Tests", NULL, NULL);
  if (NULL == pSuite2) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  /* Третий suite - специальные символы */
  pSuite3 = CU_add_suite("Special Characters Tests", NULL, NULL);
  if (NULL == pSuite3) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  /* Добавляем тесты в первый suite */
  if ((NULL == CU_add_test(pSuite1, "test of RevertString function", testRevertString))) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  /* Добавляем тесты во второй suite */
  if ((NULL == CU_add_test(pSuite2, "test edge cases", testEdgeCases))) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  /* Добавляем тесты в третий suite */
  if ((NULL == CU_add_test(pSuite3, "test special characters", testSpecialCharacters))) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  /* Run all tests using the CUnit Basic interface */
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}
