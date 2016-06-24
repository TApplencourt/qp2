#!/usr/bin/env python

"""Usage: sequoia.py [root...] < <graphiz_graph>

Prints all the ancestors of the root from the graphiz_graph
"""

import re
p = re.compile(r'\s*(\S*)\s*->\s*(\S*)\s*;$', re.MULTILINE)

def get_graph():
    data = [re.findall(p, line) for line in sys.stdin]
    
    from collections import defaultdict
    graph = defaultdict(set)
    for line in data:
        try:
            node, child = line[0]
        except IndexError:
            pass
        else:
            graph[node] |= set([child])
    return graph

def bfs(graph, start):
    visited, queue = set(), [start]
    while queue:
        vertex = queue.pop(0)
        if vertex not in visited:
            visited.add(vertex)
            queue.extend(graph[vertex] - visited)
    return visited - set([start])


if __name__ == '__main__':
    import sys
    roots = sys.argv[1:]
    graph = get_graph()

    children = set()
    for root in roots:
        children |= bfs(graph,root)

    print "\n".join(children)
