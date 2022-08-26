CREATE TABLE IF NOT EXISTS `responses` (
    `key` TEXT NOT NULL,
    `value` TEXT NOT NULL
);

CREATE UNIQUE INDEX IF NOT EXISTS
    `response_key_index`
    ON `responses` (`key`);