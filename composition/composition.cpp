#include <iostream>

void Delete(const int* array) { delete[] array; }
void Delete(const bool* array) { delete[] array; }
void Delete(int** array, int size) {
  for (int i = 0; i < size; ++i) {
    delete[] array[i];
  }
  delete[] array;
}

long long Answer(int level, int* length, int** array, bool* is_used,
                 int max_level) {
  long long sum = 0;
  if (level == max_level) {
    for (int i = 0; i < length[level]; ++i) {
      if (!is_used[i]) {
        sum += array[level][i];
      }
    }
  } else {
    for (int i = 0; i < length[level]; ++i) {
      if (!is_used[i]) {
        is_used[i] = true;
        sum += array[level][i] *
               Answer(level + 1, length, array, is_used, max_level);
        is_used[i] = false;
      }
    }
  }
  return sum;
}

long long Solve(int argc, char** argv) {
  int* length = new int[argc - 1];
  int** array = new int*[argc - 1];
  int max_length = 0;
  for (int i = 0; i < (argc - 1); ++i) {
    length[i] = atoi(argv[i + 1]);
    max_length = std::max(max_length, length[i]);
    array[i] = new int[length[i]];
    for (int j = 0; j < length[i]; ++j) {
      std::cin >> array[i][j];
    }
  }
  bool* is_used = new bool[max_length]();

  long long ans = Answer(0, length, array, is_used, argc - 2);

  Delete(length);
  Delete(array, argc - 1);
  Delete(is_used);

  return ans;
}

int main(int argc, char* argv[]) {
  std::cout << Solve(argc, argv);
  return 0;
}