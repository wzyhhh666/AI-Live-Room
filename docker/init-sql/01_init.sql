CREATE DATABASE IF NOT EXISTS chatroom_db DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE chatroom_db;

CREATE TABLE IF NOT EXISTS `user` (
    `user_id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '用户ID',
    `nickname` VARCHAR(64) NOT NULL DEFAULT '' COMMENT '昵称',
    `avatar_url` VARCHAR(512) DEFAULT '' COMMENT '头像URL',
    `role` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '角色: 0普通用户 1主播 2管理员',
    `status` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '状态: 0禁用 1正常',
    `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`user_id`),
    INDEX `idx_nickname` (`nickname`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='用户表';

CREATE TABLE IF NOT EXISTS `room` (
    `room_id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '房间ID',
    `owner_id` BIGINT UNSIGNED NOT NULL COMMENT '房主用户ID',
    `room_name` VARCHAR(128) NOT NULL DEFAULT '' COMMENT '房间名称',
    `cover_url` VARCHAR(512) DEFAULT '' COMMENT '封面图URL',
    `max_online` INT UNSIGNED NOT NULL DEFAULT 50000 COMMENT '最大在线人数',
    `status` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '状态: 0关闭 1直播中 2封禁',
    `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`room_id`),
    INDEX `idx_owner` (`owner_id`),
    INDEX `idx_status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='直播间表';

CREATE TABLE IF NOT EXISTS `danmaku` (
    `danmaku_id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '弹幕ID',
    `room_id` BIGINT UNSIGNED NOT NULL COMMENT '房间ID',
    `user_id` BIGINT UNSIGNED NOT NULL COMMENT '发送者用户ID',
    `content` VARCHAR(512) NOT NULL DEFAULT '' COMMENT '弹幕内容',
    `color` VARCHAR(16) DEFAULT '#FFFFFF' COMMENT '字体颜色',
    `font_size` TINYINT UNSIGNED DEFAULT 25 COMMENT '字号',
    `is_blocked` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '是否被屏蔽: 0正常 1已屏蔽',
    `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`danmaku_id`),
    INDEX `idx_room_time` (`room_id`, `created_at`),
    INDEX `idx_user` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='弹幕记录表';

CREATE TABLE IF NOT EXISTS `gift_record` (
    `record_id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '记录ID',
    `room_id` BIGINT UNSIGNED NOT NULL COMMENT '房间ID',
    `sender_id` BIGINT UNSIGNED NOT NULL COMMENT '送礼者用户ID',
    `receiver_id` BIGINT UNSIGNED NOT NULL COMMENT '收礼者(主播)用户ID',
    `gift_id` INT UNSIGNED NOT NULL COMMENT '礼物ID',
    `gift_count` SMALLINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '数量',
    `total_price` DECIMAL(10,2) NOT NULL DEFAULT 0.00 COMMENT '总价值',
    `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`record_id`),
    INDEX `idx_room_time` (`room_id`, `created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='礼物记录表';

SELECT '✅ 数据库表创建完成!' AS result;
