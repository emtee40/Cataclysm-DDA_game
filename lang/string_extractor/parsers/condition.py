from ..write_text import write_text


def parse_condition(conditions, origin):
    if not type(conditions) is list:
        conditions = [conditions]
    for cond in conditions:
        if type(cond) is dict:
            if "u_query" in cond:
                write_text(cond["u_query"], origin,
                           comment="Query message shown in a popup")
            if "npc_query" in cond:
                write_text(cond["npc_query"], origin,
                           comment="Query message shown in a popup")
            if "and" in cond:
                parse_condition(cond["and"], origin)
            if "or" in cond:
                parse_condition(cond["or"], origin)
            if "not" in cond:
                parse_condition(cond["not"], origin)
