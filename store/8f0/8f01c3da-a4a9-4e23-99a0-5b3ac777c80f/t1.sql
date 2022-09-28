ATTACH TABLE _ UUID '1e549b32-1b00-4251-a8c0-746a345f0be2'
(
    `c1` UInt32,
    `c2` UInt32,
    INDEX idx c2 TYPE minmax GRANULARITY 1
)
ENGINE = MergeTree
PRIMARY KEY c1
ORDER BY c1
SETTINGS index_granularity = 8192
