#include <stdio.h>
#include <stdlib.h>

// ここではplusOneというint型の引数を取る変数が(return n+1)をする定義となっている
// 下で出てくるintを返す関数へのポインタ変数との違いは単に内容が定義されているかしないかとなる
int plusOne(int n) {
  return n + 1;
}

int main() {
  int (*f)(int); // 単に関数ポインタ変数の宣言
  int result;

  f = plusOne;
  result = f(5);
  printf("result = %d\n", result);
  exit(0);
}
