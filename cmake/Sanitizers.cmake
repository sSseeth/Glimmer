# glimmer_enable_sanitizers(<target>)
#
# AddressSanitizer finds use-after-free, buffer overflows and leaks.
# UndefinedBehaviorSanitizer finds signed overflow, bad casts, misaligned
# access, etc. Both are cheap enough to run on every CI build of the core.
function(glimmer_enable_sanitizers target)
  if(NOT GLIMMER_ENABLE_SANITIZERS)
    return()
  endif()
  if(MSVC)
    target_compile_options(${target} PRIVATE /fsanitize=address)
    return()
  endif()
  set(flags -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_compile_options(${target} PRIVATE ${flags})
  target_link_options(${target} PRIVATE ${flags})
endfunction()
