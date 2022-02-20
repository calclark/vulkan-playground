project(
  'fmt',
  'cpp',
  default_options: [
    'warning_level=0'
  ]
)

inc = include_directories(
  'include',
  is_system: true
)

fmt = static_library(
  'fmt',
  'src/format.cpp',
  include_directories : inc,
)

fmt_dep = declare_dependency(
  include_directories :inc,
  link_with: fmt
)
