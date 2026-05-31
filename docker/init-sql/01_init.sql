CREATE DATABASE IF NOT EXISTS chatroom_db DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE chatroom_db;

CREATE TABLE IF NOT EXISTS `users` (
    `id` INT AUTO_INCREMENT PRIMARY KEY,
    `username` VARCHAR(50) NOT NULL UNIQUE,
    `nickname` VARCHAR(100),
    `avatar` VARCHAR(255),
    `role` TINYINT DEFAULT 0 COMMENT '0=观众, 1=主播, 2=管理员',
    `token` VARCHAR(255),
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='用户表';

CREATE TABLE IF NOT EXISTS `rooms` (
    `id` INT AUTO_INCREMENT PRIMARY KEY,
    `room_name` VARCHAR(100) NOT NULL,
    `host_id` INT,
    `host_name` VARCHAR(100),
    `title` VARCHAR(255),
    `cover_image` VARCHAR(255),
    `online_count` INT DEFAULT 0,
    `state` ENUM('idle', 'living', 'closed') DEFAULT 'idle',
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (`host_id`) REFERENCES `users`(`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='直播间表';

CREATE TABLE IF NOT EXISTS `danmaku_messages` (
    `id` BIGINT AUTO_INCREMENT PRIMARY KEY,
    `room_id` INT NOT NULL,
    `user_id` INT NOT NULL,
    `username` VARCHAR(100) NOT NULL,
    `content` TEXT NOT NULL,
    `color` VARCHAR(20) DEFAULT '#00ff41',
    `type` ENUM('normal', 'gift', 'system') DEFAULT 'normal',
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX `idx_room_created` (`room_id`, `created_at`),
    FOREIGN KEY (`room_id`) REFERENCES `rooms`(`id`),
    FOREIGN KEY (`user_id`) REFERENCES `users`(`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='弹幕消息表';

CREATE TABLE IF NOT EXISTS `gift_record` (
    `record_id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '记录ID',
    `room_id` INT NOT NULL COMMENT '房间ID',
    `sender_id` INT NOT NULL COMMENT '送礼者用户ID',
    `sender_name` VARCHAR(100) NOT NULL DEFAULT '' COMMENT '送礼者昵称',
    `receiver_id` INT NOT NULL COMMENT '收礼者(主播)用户ID',
    `gift_id` INT UNSIGNED NOT NULL COMMENT '礼物ID',
    `gift_name` VARCHAR(100) NOT NULL DEFAULT '' COMMENT '礼物名称',
    `gift_count` SMALLINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '数量',
    `total_price` DECIMAL(10,2) NOT NULL DEFAULT 0.00 COMMENT '总价值',
    `effect_type` VARCHAR(50) NOT NULL DEFAULT 'normal' COMMENT '特效类型',
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX `idx_room_time` (`room_id`, `created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='礼物记录表';

CREATE TABLE IF NOT EXISTS `gift_config` (
    `id` INT AUTO_INCREMENT PRIMARY KEY,
    `gift_name` VARCHAR(50) NOT NULL,
    `gift_icon` VARCHAR(10) NOT NULL COMMENT 'emoji图标',
    `price` DECIMAL(10,2) NOT NULL DEFAULT 0.00,
    `effect_type` VARCHAR(20) DEFAULT 'normal' COMMENT '特效类型: normal/explosion/rain/rocket',
    `sort_order` INT DEFAULT 0,
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='礼物配置表';

INSERT IGNORE INTO `gift_config` (`id`, `gift_name`, `gift_icon`, `price`, `effect_type`, `sort_order`) VALUES
(1, '荧光棒', '🪄', 1.00, 'normal', 1),
(2, '点赞', '👍', 2.00, 'normal', 2),
(3, '鲜花', '🌸', 5.00, 'rain', 3),
(4, '跑车', '🏎️', 50.00, 'rocket', 4),
(5, '火箭', '🚀', 100.00, 'explosion', 5),
(6, '嘉年华', '🎪', 500.00, 'explosion', 6);

CREATE TABLE IF NOT EXISTS `room_members` (
    `id` INT AUTO_INCREMENT PRIMARY KEY,
    `room_id` INT NOT NULL,
    `user_id` INT NOT NULL,
    `username` VARCHAR(100) NOT NULL,
    `join_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `leave_time` TIMESTAMP NULL DEFAULT NULL,
    UNIQUE KEY `uk_room_user` (`room_id`, `user_id`),
    FOREIGN KEY (`room_id`) REFERENCES `rooms`(`id`),
    FOREIGN KEY (`user_id`) REFERENCES `users`(`id`),
    INDEX `idx_room_join` (`room_id`, `join_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='房间成员表';

INSERT IGNORE INTO `users` (`id`, `username`, `nickname`, `avatar`, `role`)
VALUES (1, 'ai_host', 'AI主播', 'https://api.dicebear.com/7.x/avataaars/svg?seed=host', 1);

INSERT IGNORE INTO `rooms` (`id`, `room_name`, `host_id`, `host_name`, `title`, `cover_image`, `online_count`, `state`)
VALUES (1, 'AI直播间', 1, 'AI主播', '🔥 AI技术前沿直播 - 实时编程演示',
        'https://api.dicebear.com/7.x/avataaars/svg?seed=host', 0, 'living');

SELECT '✅ 数据库表创建完成!' AS result;
