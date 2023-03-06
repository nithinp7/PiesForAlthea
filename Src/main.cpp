#include "DemoScene.h"

#include <Althea/Application.h>

#include <iostream>

using namespace PiesForAlthea;
using namespace AltheaEngine;

int main() {
  Application app("../..", "../../Extern/Althea");
  app.createGame<DemoScene>();

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}