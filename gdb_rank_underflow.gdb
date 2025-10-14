set pagination off
set confirm off
set debuginfod enabled off
set $shift_recorded = 0
set $shift_value = (long long)0
set $shift_is_infinity = 0
break /home/rdmerrio/gits/ranked_belief/include/ranked_belief/operations/observe.hpp:58
commands
  silent
  printf "\nnormalize_with_shift breakpoint hit\n"
  set $current_raw = current.get()
  printf "current raw pointer: %p\n", $current_raw
  if ($current_raw == 0)
    printf "current was null -- aborting\n"
    quit
  end
  set $curr_rank = $current_raw->rank_
  set $curr_is_inf = $curr_rank.is_infinity_
  if ($curr_is_inf)
    printf "current rank: is_infinity=1 value=<n/a>\n"
  else
    set $curr_val = $curr_rank.value_
    printf "current rank: is_infinity=0 value=%llu\n", (unsigned long long)$curr_val
  end
  if ($shift_recorded == 0)
    set $shift_recorded = 1
    set $shift_is_infinity = $curr_is_inf
    if ($curr_is_inf)
      printf "recorded shift amount: infinity\n"
    else
      set $shift_value = (long long)$curr_val
      printf "recorded shift amount: %lld\n", $shift_value
    end
  end
  if ($curr_is_inf)
    printf "delta vs shift: <infinite current>\n"
  else
    if ($shift_is_infinity)
      printf "delta vs shift: <shift infinity>\n"
    else
      set $delta = (long long)$curr_val - $shift_value
      printf "delta vs shift: %lld\n", $delta
    end
  end
  continue
end
run
quit