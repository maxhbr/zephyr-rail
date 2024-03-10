#!/usr/bin/env nix-shell
#! nix-shell -i bash -p mermaid-cli entr

mermaid() {
    # S_PARENT_INTERACTIVE;
  cat <<EOF |mmdc -i - -o mermaid.StateMachine.svg -t neutral -b transparent
flowchart LR;
    S0((S0));

    subgraph S_PARENT_INTERACTIVE
      direction TB 
      S_INTERACTIVE_MOVE;
      S_INTERACTIVE_MOVE -->|press right| S_INTERACTIVE_PRE_STACKING;
      S_INTERACTIVE_PRE_STACKING -->|press left| S_INTERACTIVE_MOVE;
    end
    S0 --> S_INTERACTIVE_MOVE;

    subgraph S_PARENT_STACKING;
      direction TB
      S_INTERACTIVE_PRE_STACKING --->|press enter| S_STACK{S_STACK\ndone?};
      S_STACK --> S_STACK_MOVE;
      S_STACK_MOVE --> S_STACK_IMG;
      S_STACK_IMG --> S_STACK;
    end
      S_STACK --->|done| S0;


    S_INTERACTIVE_MOVE -->|press\n- up: inc step\n- down: dec step\n...| S_INTERACTIVE_MOVE;
    S_INTERACTIVE_PRE_STACKING -->|press\n- up: inc nmbr\n- down: dec nmbr\n...| S_INTERACTIVE_PRE_STACKING;
EOF
}

if [[ "$1" == "run" ]]; then
  mermaid
else
  echo "$0" | entr "$0" run
fi
