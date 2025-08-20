labs_sources = [
  'iterators.cpp',
]

foreach s : labs_sources
  executable(
    s.split('.')[0], # Using split as a fallback, though explicit listing is preferred.
    s,
    install: false,
  )
endforeach
